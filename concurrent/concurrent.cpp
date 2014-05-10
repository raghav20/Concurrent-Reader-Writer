#include <iostream>
#include <thread>
#include <vector>
#include <string>
#include <mutex>
#include <cstdio>
#include <cassert>
#include <cstring>
#define MAX_FILE_SZ 1000
#define CHUNK_SZ 10
#define Q_SIZE 10
using namespace std;
 
// Final data to be written to file.
bool isAlreadyWritten[MAX_FILE_SZ] = {0};
char writeBuffer[MAX_FILE_SZ][CHUNK_SZ]; // MAX_FILE_SZ Chunks of 10 bytes.
 
// Data read from file. Chunk of 10 bytes.
class RData
{
  int byteOffset;
  string data;
  public:
  RData(int offset, char* aData)
  {
    byteOffset = offset;
    data.clear();
    data = aData;
  }
  RData(RData& aData)
  {
    data.clear();
    data = aData.data;
    byteOffset = aData.byteOffset;
  }
  const char* getData() const { return data.c_str(); }
  int getOffset() const { return byteOffset; }
  int getLength() const { return data.size(); }
};
 
// Mutex used to control access to critical regions.
mutex sendMtx;
mutex recvMtx;
mutex global;
 
 
// Class to monitor execution of readers
class RMonitor
{
  int count;
  public:
  RMonitor() { count = 0; }
  void setCount(int cnt) { count = cnt; }
  void deregisterThread(int tid) { count -= tid; }
  bool done() { return (count == 0); }
} rMonitor;
 
// Message Queue Implementation
class MesgQueue
{
  RData* qData[Q_SIZE];
//indicates the next vacant position in queue.
  int dataIdx;
  public:
  MesgQueue()
  {
    dataIdx = 0;
    for (int i = 0; i < Q_SIZE; i++)
      qData[i] = NULL;
  }
 
void appendQ(RData* pData)
{
  global.lock();
  qData[dataIdx++] = pData;
  global.unlock();
}
RData* deleteQ()
{
  global.lock();
  RData* pData = qData[dataIdx - 1];
  dataIdx--;
  global.unlock();
  return pData;
}
 
void send(RData data)
{
// busy waiting till the buffer has space again.
  while (dataIdx >= Q_SIZE);
  appendQ(new RData(data));
}
 
RData* recv()
{
// busy waiting till the buffer has something to read from
// and threads have still got job to do.
  if (dataIdx == 0)
  {
    if (rMonitor.done())
    return NULL;
    while (dataIdx == 0);
  }
  RData* data = deleteQ();
// caller is responsible for cleanup 
  return data;
}
} mesgQ;
 
// Function executed by all readers
void readData(const char* inputFile, int thIdx)
{
  FILE* fp = fopen(inputFile, "rb");
  assert(fp);
  int chunkIdx = 0;
  while (!feof(fp))
  {
// temporary buffer to store read data.
    char temp[CHUNK_SZ + 1] = "";
    fread(temp, CHUNK_SZ, 1, fp);
    RData rData(chunkIdx, temp);
    chunkIdx++;
    mesgQ.send(rData);
  }
    rMonitor.deregisterThread(thIdx);
    fclose(fp);
}
 
// Function executed by all writers
void writeData()
{
// While all message is received.
    RData* data = mesgQ.recv();
    while (data)
    {
      int offset = data->getOffset();
// See if this data is duplicate
      if (isAlreadyWritten[offset])
      {
       // discard.
      }
      else
      {
         char* destn = writeBuffer[offset];
         memcpy(destn, data->getData(), data->getLength());
         isAlreadyWritten[offset] = true;
      }
// We are done with the data now. Safe to delete.
      delete data;
      data = mesgQ.recv();
   }
}
 
int main(int argc, char* argv[])
{
  if (argc != 3)
  {
    cout << "Usage: " << argv[0] << " numReaders numWriters" << endl;
    exit(1);
  }
  const char* inputFile = "input.txt";
  int numReaders = atoi(argv[1]);
  int numWriters = atoi(argv[2]);
// We assign a hypothetical id to reader threads as 1,2,3 ..n
  int sum = numReaders * (numReaders + 1) / 2;
  rMonitor.setCount(sum);
// Store all created threads
  vector<std::thread> threads;
// Create Readers
  for (int i = 0; i < numReaders; i++)
     threads.push_back(std::thread(readData, inputFile, i + 1));
// Create Writers
  for (int i = 0; i < numWriters; i++)
     threads.push_back(std::thread(writeData));
 
// Wait for all of them to finish
  for(auto& t : threads)
     t.join();
 
// Now write the output file.
  FILE* fp = fopen("output.txt", "wb");
  int offset = 0;
  while (isAlreadyWritten[offset])
  {
     fwrite(writeBuffer[offset], CHUNK_SZ, 1, fp);
     offset++;
  }
  fclose(fp);
}
