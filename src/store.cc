#include <grpc++/grpc++.h>
#include <grpc/support/log.h>
#include "ThreadPool.h"
#include "store.grpc.pb.h"
#include <iostream>

using grpc::Server;
using grpc::ServerAsyncResponseWriter;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerCompletionQueue;
using grpc::Status;
using store::Store;
using store::ProductInfo;
using store::ProductQuery;
using store::ProductReply;


class Store final{
public:
    ~Store(){
        //server_ and cq_ is populated in run()
        server_->Shutdown();
        //shutdown server before shutdown cq
        cq_->Shutdown();
    }

    Store(size_t n_thread):pool_(n_thread){

    }

    void Run(){
        //use 50056 since 50051~50055 is taken by vendors
        std::string server_address_port("localhost:50056");

        ServerBuilder sb;//default initializer
        sb.AddListeningPort(server_address_port,grpc::InsecureServerCredentials());
        sb.RegisterService(&service_);//register service;(no idea what is happening in the box)
        cq_ = sb.AddCompletionQueue();
        server_ = sb.BuildAndStart();
        std::cout <<"Server listening on "<<server_address_port<<std::endl;

        HandlerRPCs();
    }



private:

    class CallData {
    public:
        CallData(ServerContext *ctx):responder_(ctx),status(TOHANDLE){}
        enum CallStatus { TOHANDLE, FINISH };
        CallStatus status;
    private:

        //seems like in our implementation, we don't need
        // Store::AsyncService* service_;

        //Cong: seems that in our implementation we don't need
        // ServerCompletionQueue* cq_;

        //seems like in our implementation, we don't need
        //ServerContext ctx_;

        // What we get from the client.
        ProductQuery request_;
        // What we send back to the client.
        ProductReply reply_;

        // The means to get back to the client.
        ServerAsyncResponseWriter<ProductReply> responder_;


    };

    void HandlerRPCs(){
        //idea is: keep receiving and adding it to queue, will be queued by threadpool

    }

    //member objects are always initialized
    std::unique_ptr<ServerCompletionQueue> cq_;

    Store::AsyncService service_;

    std::unique_ptr<Server> server_;

    ThreadPool pool_;

    //what is this for ???
    ServerContext ctx_;



};

int main(int argc, char** argv) {
	Store store(4);
	store.Run();
	std::cout << "I 'm not ready yet!" << std::endl;
	return EXIT_SUCCESS;
}

