#include "control.h"
#include <stdio.h>
#include <grpcpp/grpcpp.h>

using grpc::ClientContext;

void doSomethingInteresting() {
    ClientContext context;
    printf("This is a test...\n");
}
