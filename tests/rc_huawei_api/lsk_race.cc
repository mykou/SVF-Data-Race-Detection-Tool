// Please compile and use RaceComb on this sample file
// clang++ lsk_race.cc -c -emit-llvm -S -g -std=c++11 -o lsk_race.ll
// rc -stat=false lsk_race.ll

/*
 * Simple race check
 * Author: dye
 * Date: 16/02/2017
 */
#include <map>
#include <memory>
#include <string.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <cstdlib>

using namespace std;

//#define USE_SHARED_PTR


class ServerRequest {
    int id;
};


enum ServerRequestMethod {
      DELETE, GET, CREATE
};

class ServerRequestProcess {
public:
    std::map<int, int*> promiseMap; // race object
    void sendRequest(const ServerRequest &request);
};

void ServerRequestProcess::sendRequest(const ServerRequest &request) {
    int *p = (int *)malloc(sizeof(int));
    int x = rand();
    promiseMap.insert(pair<int, int*>(x, p)); // race access here
}

class LSKClient {
public:
    void del(const char *key, unsigned int keyLen, int version=-1);
    void del(const char *key, unsigned int keyLen, int version, bool sharedModel);
    void del(unsigned int raftChainId, const char *key, unsigned int keyLen, int version);
    void sendServerRequest(ServerRequestMethod method, unsigned int raftChainId, const char *key, unsigned int keyLen, int version);
    void sendSharedModelServerRequest(ServerRequestMethod method, const char *key, unsigned int keyLen, const char *value, unsigned int valueLen, int version);

#ifdef USE_SHARED_PTR
    shared_ptr<ServerRequestProcess> serverRequestProcess;
#else
    ServerRequestProcess *serverRequestProcess;
#endif

    LSKClient() {
#ifdef USE_SHARED_PTR
        serverRequestProcess = shared_ptr<ServerRequestProcess>(new ServerRequestProcess());
#else
        serverRequestProcess = new ServerRequestProcess();
#endif
    }
};

void LSKClient::del(const char *key, unsigned int keyLen, int version) {
    del(0, key, keyLen, version);
}

void LSKClient::del(const char *key, unsigned int keyLen, int version, bool sharedModel) {

    sharedModel ? 
           sendSharedModelServerRequest(ServerRequestMethod::DELETE, key, keyLen, NULL, 0, version) : del(key, keyLen, version);
}

void LSKClient::del(unsigned int raftChainId, const char *key, unsigned int keyLen, int version) {
    sendServerRequest(ServerRequestMethod::DELETE, raftChainId, key, keyLen, version);
}

void LSKClient::sendSharedModelServerRequest(ServerRequestMethod method, const char *key, unsigned int keyLen, const char *value, unsigned int valueLen, int version) {
   ServerRequest request;
   serverRequestProcess->sendRequest(request);
}

void LSKClient::sendServerRequest(ServerRequestMethod method, unsigned int raftChainId, const char *key, unsigned int keyLen, int version) {
   ServerRequest request;
   serverRequestProcess->sendRequest(request);
}


LSKClient *g_lskClient = NULL;
LSKClient *g_lskJobDict = NULL;


int kvDelete(int type, char *key, unsigned int keyLen) {
   switch(type) {
   case 0: 
      g_lskClient->del((const char *)key, keyLen);
      break;
   case 1: 
      g_lskJobDict->del((const char *)key, keyLen);
      break;
   } 
   return 0;
}


int H_LTaskCacheDelete(char *key, unsigned int keyLen) {
    return kvDelete(0, key, keyLen);
}


void *kvdel(void *ptr) {
    H_LTaskCacheDelete((char*)"1", 2);
    //H_LTaskCacheDelete((char*)ptr, strlen((char*)ptr) + 1);
    return NULL;
}

#define NUM_THREAD 4

int main(int argc, char *argv[]) {

    pthread_t tid[NUM_THREAD];
    char *strLists[] = {(char*)"one", (char*)"two", (char*)"3", (char*)"4"};

    g_lskClient = new LSKClient();
    g_lskJobDict = new LSKClient();

    for (int i = 0; i < NUM_THREAD; ++i) {
        pthread_create(&tid[i], NULL, &kvdel, strLists[i]);
    } 

   for (int i = 0; i < NUM_THREAD; ++i) 
       pthread_join(tid[i], NULL);

    return 0;
}
