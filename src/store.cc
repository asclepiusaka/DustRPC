#include <grpc++/grpc++.h>
#include <grpc/support/log.h>
#include "ThreadPool.h"
#include "store.grpc.pb.h"
#include "vendor.grpc.pb.h"
#include <iostream>
#include <fstream>

using grpc::Channel;
using grpc::Server;
using grpc::ServerAsyncResponseWriter;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerCompletionQueue;
using grpc::CompletionQueue;
using grpc::ClientContext;
using grpc::ClientAsyncResponseReader;
using grpc::Status;
using vendor::Vendor;
using vendor::BidQuery;
using vendor::BidReply;
using store::Store;
using store::ProductInfo;
using store::ProductQuery;
using store::ProductReply;


class StoreImp final{
public:
    ~StoreImp(){
        //server_ and cq_ is populated in run()
        server_->Shutdown();
        //shutdown server before shutdown cq
        cq_->Shutdown();
    }



    StoreImp(size_t n_thread, std::string vendor_file): pool_(n_thread){
        vendor_addr_ = getVendorList(vendor_file);
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

    std::vector<std::string> getVendorList(std::string &file_url) {
        std::vector<std::string> vendorList;
        std::ifstream file(file_url);
        if(file.is_open()){
            std::string serverAddr;
            while(std::getline(file,serverAddr)){
                vendorList.push_back(serverAddr);
            }
            file.close();
            return vendorList;
        }else{
            std::cerr<<"Failed to open vendor file "<<file_url<<std::endl;
            exit(-1);
        }
    }

    class CallData {
    public:
        ServerContext sc;
        CallData(std::vector<std::string> addr):vendorList_(addr),responder_(&sc),status(TOHANDLE){}
        enum CallStatus { TOHANDLE, FINISH };
        CallStatus status;
        void process(){
            //process the request from client;
            //process() should change the state of CallData from TOHANDLE to FINISH;
            //should use a group of vendors to query the information that's asked by the client.
            status = FINISH;
            CompletionQueue vendor_cq;
            //send all the query
            for(std::string addr:vendorList_){
                VendorQueryAgent *vendor = new VendorQueryAgent(grpc::CreateChannel(
                        addr, grpc::InsecureChannelCredentials()),&vendor_cq);
                vendor->getProductBid(request_);
            }

            for(int i=0;i<5;i++) {
                void *got_tag;
                bool ok = false;
                //wait the five vendor query to get back. hanlde them and add to response data structure;
                GPR_ASSERT(vendor_cq.Next(&got_tag, &ok));


                // ... and that the request was completed successfully. Note that "ok"
                // corresponds solely to the request for updates introduced by Finish().
                GPR_ASSERT(ok);

                // Act upon the status of the actual RPC.
                if (!static_cast<VendorQueryAgent*>(got_tag)->status.ok()) {
                    std::cout << static_cast<VendorQueryAgent*>(got_tag)->status.error_code() << ": "
                              << static_cast<VendorQueryAgent*>(got_tag)->status.error_message()
                              << std::endl;
                }
                BidReply *bidreply = &(static_cast<VendorQueryAgent*>(got_tag)->reply);

                ProductInfo *product = reply_.add_products();
                product->set_price(bidreply->price());
                product->set_vendor_id(bidreply->vendor_id());
                delete static_cast<VendorQueryAgent*>(got_tag);

            }
            responder_.Finish(reply_,Status::OK,this);





        }

        // What we get from the client.
        ProductQuery request_;
        // What we send back to the client.
        ProductReply reply_;

        // The means to get back to the client.
        ServerAsyncResponseWriter<ProductReply> responder_;
    private:
        class VendorQueryAgent {
        public:


            explicit VendorQueryAgent(std::shared_ptr<Channel> channel,CompletionQueue* shared_cq)
                    : stub_(Vendor::NewStub(channel)),cq_(shared_cq) {}

            // Assembles the client's payload, sends it and presents the response back
            // from the server.
            void getProductBid(const ProductQuery& request) {
                BidQuery query;
                query.set_product_name(request.product_name());



                // Context for the client. It could be used to convey extra information to
                // the server and/or tweak certain RPC behaviors.
                ClientContext context;

                // The producer-consumer queue we use to communicate asynchronously with the
                // gRPC runtime.


                // Storage for the status of the RPC upon completion.


                // stub_->AsyncSayHello() performs the RPC call, returning an instance we
                // store in "rpc". Because we are using the asynchronous API, we need to
                // hold on to the "rpc" instance in order to get updates on the ongoing RPC.
                std::unique_ptr<ClientAsyncResponseReader<BidReply> > rpc(
                        stub_->PrepareAsyncgetProductBid(&context, query, cq_));

                rpc->StartCall();

                // Request that, upon completion of the RPC, "reply" be updated with the
                // server's response; "status" with the indication of whether the operation
                // was successful. Tag the request with the integer 1.
                rpc->Finish(&reply, &status, (void*)this);

            }

            BidReply reply;

            Status status;

        private:
            // Out of the passed in Channel comes the stub, stored here, our view of the
            // server's exposed services.
            std::unique_ptr<Vendor::Stub> stub_;
            CompletionQueue* cq_;
        };

        //seems like in our implementation, we don't need
        // Store::AsyncService* service_;

        //Cong: seems that in our implementation we don't need
        // ServerCompletionQueue* cq_;

        //seems like in our implementation, we don't need
        //ServerContext ctx_;

        std::vector<std::string> vendorList_;

    };

    void HandlerRPCs(){
        //idea is: keep receiving and adding it to queue, will be queued by threadpool



        CallData* to_fill = new CallData(vendor_addr_);
        void* tag;
        bool ok;

        service_.RequestgetProducts(&(to_fill)->sc, &(to_fill->request_), &(to_fill->responder_), cq_.get(), cq_.get(),
                                      to_fill);
            //doesn't block here.
            //CallData can be in two states
        while(true){
            GPR_ASSERT(cq_->Next(&tag,&ok));
            GPR_ASSERT(ok);
            CallData* returned = static_cast<CallData*>(tag);
            if(returned->status == CallData::TOHANDLE){
                //lambda expression to provide work for thread.
                pool_.enqueue([returned]() { returned->process(); });
                CallData* to_fill_new = new CallData(vendor_addr_);
                service_.RequestgetProducts(&(to_fill_new)->sc, &(to_fill_new->request_), &(to_fill_new->responder_), cq_.get(), cq_.get(),
                                            to_fill_new);
            }else{
                delete returned;
            }

        }

    }

    //member objects are always initialized
    std::unique_ptr<ServerCompletionQueue> cq_;

    Store::AsyncService service_;

    std::unique_ptr<Server> server_;

    ThreadPool pool_;


    std::vector<std::string> vendor_addr_;



};

int main(int argc, char** argv) {
    std::string vendor_file(argv[2]);
	StoreImp store(4,vendor_file);
	store.Run();
	std::cout << "I 'm not ready yet!" << std::endl;
	return EXIT_SUCCESS;
}

