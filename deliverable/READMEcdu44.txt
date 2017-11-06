store implementation:
HandleRPCs:
The main logic of store is in function HandlerRPCs(), this function has a forever loop and it listens to Server Comletation Queue, it populates CallData with new clientRequest and takes the duty to free finished CallData class. it pass callData->process() routine to threadpool for executation.
Process():
this routine changes CallData's status to FINISH, build connections to vendors and send querys. Vendor querys share the same thread-local completion queue, After all the querys are sent, the thread will waiting on the completion queue for all the bids to get back. after adding all bids from vendors, it will notify grpc runtime to send the reply to client.

store takes 3 different variable, store [number thread] [vendor_file] [listening port] i.e.:
./store 4 ./vendor_addresses.txt localhost:50056
