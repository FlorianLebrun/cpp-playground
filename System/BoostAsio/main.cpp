#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <intrin.h>
#include <iostream>
#include <boost/asio.hpp>

void print(const boost::system::error_code& /*e*/)
{
   std::cout << "Hello, world!" << std::endl;
}


boost::asio::io_service io;

struct EngineTask {

};

struct EngineContext {
   boost::asio::steady_timer timer;
   EngineTask* task;
   EngineContext() : timer(io, boost::asio::chrono::seconds(1)) {

   }
   void run_timer(const boost::system::error_code& e) {
      if (e.failed()) std::cout << "canceled: ";
      std::cout << "Hello, world!" << std::endl;
   }
};


struct TimerDispatcher {
   std::thread thread;

   TimerDispatcher()
      : thread([this]() { this->tick(); }) {
   }
   void schedule(EngineContext* ctx) {

      ctx->timer.expires_from_now(boost::asio::chrono::seconds(10));
      ctx->timer.async_wait([ctx](const boost::system::error_code& e) { ctx->run_timer(e); });

      ctx->timer.expires_from_now(boost::asio::chrono::seconds(10));
      ctx->timer.async_wait([ctx](const boost::system::error_code& e) { ctx->run_timer(e); });

      ctx->timer.expires_from_now(boost::asio::chrono::seconds(1));
      ctx->timer.async_wait([ctx](const boost::system::error_code& e) { ctx->run_timer(e); });
   }
private:
   void tick() {
      boost::asio::io_service::work work(io);
      io.run();
      std::cout << "thread end!" << std::endl;
   }
};

int main()
{
   TimerDispatcher dispatcher;
   EngineContext ctx_1;
   dispatcher.schedule(&ctx_1);

   Sleep(1000000000);
   return 0;
}


