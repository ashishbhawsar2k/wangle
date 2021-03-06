// Copyright 2004-present Facebook.  All rights reserved.
#pragma once

namespace folly { namespace wangle {

template <typename Pipeline>
void AcceptRoutingHandler<Pipeline>::read(Context* ctx, void* conn) {
  populateAcceptors();

  uint64_t connId = nextConnId_++;
  auto socket = std::shared_ptr<folly::AsyncSocket>(
      reinterpret_cast<folly::AsyncSocket*>(conn),
      folly::DelayedDestruction::Destructor());

  // Create a new routing pipeline for this connection to read from
  // the socket until it parses the routing data
  folly::DefaultPipeline::UniquePtr routingPipeline(
      new folly::DefaultPipeline);
  routingPipeline->addBack(folly::wangle::AsyncSocketHandler(socket));
  routingPipeline->addBack(routingHandlerFactory_->newHandler(connId, this));
  routingPipeline->finalize();

  routingPipeline->transportActive();
  routingPipelines_[connId] = std::move(routingPipeline);
}

template <typename Pipeline>
void AcceptRoutingHandler<Pipeline>::onRoutingData(
    uint64_t connId, RoutingDataHandler::RoutingData& routingData) {
  // Get the routing pipeline corresponding to this connection
  auto routingPipelineIter = routingPipelines_.find(connId);
  DCHECK(routingPipelineIter != routingPipelines_.end());
  auto routingPipeline = std::move(routingPipelineIter->second);
  routingPipelines_.erase(routingPipelineIter);

  // Fetch the socket from the pipeline and pause reading from the
  // socket
  auto socket = std::dynamic_pointer_cast<folly::AsyncSocket>(
      routingPipeline->getTransport());
  routingPipeline->transportInactive();
  socket->detachEventBase();

  // Hash based on routing data to pick a new acceptor
  uint64_t hash = std::hash<std::string>()(routingData.routingData);
  auto acceptor = acceptors_[hash % acceptors_.size()];

  // Switch to the new acceptor's thread
  auto mwRoutingData = folly::makeMoveWrapper<RoutingDataHandler::RoutingData>(
      std::move(routingData));
  acceptor->getEventBase()->runInEventBaseThread([=]() mutable {
    socket->attachEventBase(acceptor->getEventBase());

    auto pipeline =
        childPipelineFactory_->newPipeline(socket, mwRoutingData->routingData);
    auto pipelinePtr = pipeline.get();
    folly::DelayedDestruction::DestructorGuard dg(pipelinePtr);

    auto connection =
        new typename folly::ServerAcceptor<Pipeline>::ServerConnection(
            std::move(pipeline));
    acceptor->addConnection(connection);

    pipelinePtr->transportActive();

    // Pass in the buffered bytes to the pipeline
    pipelinePtr->read(mwRoutingData->bufQueue);
  });
}

template <typename Pipeline>
void AcceptRoutingHandler<Pipeline>::onError(uint64_t connId) {
  // Delete the pipeline. This will close and delete the socket as well.
  routingPipelines_.erase(connId);
}

template <typename Pipeline>
void AcceptRoutingHandler<Pipeline>::populateAcceptors() {
  if (!acceptors_.empty()) {
    return;
  }
  CHECK(server_);
  server_->forEachWorker(
      [&](folly::Acceptor* acceptor) { acceptors_.push_back(acceptor); });
}

}} // namespace folly::wangle
