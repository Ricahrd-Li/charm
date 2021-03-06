#include "within_node_bcast.decl.h"

#define ITERATIONS 100
#define DATA_SIZE 1024
#define TOTAL_ELEMENTS 128

CProxy_TestGroup gProxy;
CProxy_TestNodeGroup ngProxy;
CProxy_TestArray aProxy;

class TestMessage : public CMessage_TestMessage {
public:
  int test_num;
  int* data;
  std::atomic<int> counter;
  CkCallback cb;

  TestMessage(int t, CkCallback c) : test_num(t), cb(c), counter(0) {}
};

class Main : public CBase_Main {
private:
  Main_SDAG_CODE
  int test_num, tests_completed;
public:
  Main(CkArgMsg* msg) : tests_completed(0) {
    delete msg;

    ngProxy = CProxy_TestNodeGroup::ckNew();
    gProxy = CProxy_TestGroup::ckNew();
    aProxy = CProxy_TestArray::ckNew(TOTAL_ELEMENTS);

    // Defined as an SDAG method in the .ci file
    thisProxy.runTests();

    // Make sure QD counters are correctly updated for WNB
    CkStartQD(CkCallback(CkIndex_Main::allComplete(), thisProxy));
  }

  void allComplete() {
    CkAssert(tests_completed == 6);
    CkExit();
  }
};

class TestNodeGroup : public CBase_TestNodeGroup {
private:
  std::atomic<int> num_elements;
public:
  TestNodeGroup() : num_elements(0) {}

  void addElement() {
    num_elements++;
  }

  void recv(TestMessage* msg) {
    // Here the expectation is that the counter is incremented once by each PE
    // and starts at 0. So my nodes sum will be n * (n - 1) / 2.
    int myExpectation = (CmiMyNodeSize() * (CmiMyNodeSize() - 1)) / 2;
    contribute(sizeof(int), &myExpectation, CkReduction::sum_int, msg->cb);

    CkBroadcastWithinNode(CkIndex_TestGroup::recv(NULL), msg, gProxy);
  }

  void recvCopy(TestMessage* msg) {
    // Here, there will be one copy of the message per PE, so each PE will
    // contribute msg->test_num, and my sum is therefor my size * msg->test_num.
    CkAssert(msg->counter.load() == msg->test_num);
    int myExpectation = msg->test_num * CkMyNodeSize();
    contribute(sizeof(int), &myExpectation, CkReduction::sum_int, msg->cb);

    CkBroadcastWithinNode(CkIndex_TestGroup::recvCopy(NULL), msg, gProxy);
  }

  void recvNoFwd(TestMessage* msg) {
    // Here the expectation is that the counter is incremented once by each PE
    // and starts at 0. So my nodes sum will be n * (n - 1) / 2.
    int myExpectation = (CmiMyNodeSize() * (CmiMyNodeSize() - 1)) / 2;
    contribute(sizeof(int), &myExpectation, CkReduction::sum_int, msg->cb);
  }

  void recvCopyNoFwd(TestMessage* msg) {
    // Here, there will be one copy of the message per PE, so each PE will
    // contribute msg->test_num, and my sum is therefor my size * msg->test_num.
    CkAssert(msg->counter.load() == msg->test_num);
    int myExpectation = msg->test_num * CkMyNodeSize();
    contribute(sizeof(int), &myExpectation, CkReduction::sum_int, msg->cb);
  }

  void recvForArray(TestMessage* msg) {
    // Here the expectation is that the counter is incremented once by each
    // array element and starts at 0. So my nodes sum will be n * (n - 1) / 2.
    int myExpectation = (num_elements * (num_elements - 1)) / 2;
    contribute(sizeof(int), &myExpectation, CkReduction::sum_int, msg->cb);
  }

  void recvCopyForArray(TestMessage* msg) {
    // Here, there will be one copy of the message per array element, so each
    // array element will contribute msg->test_num, and my sum is therefor my
    // num_elements * msg->test_num.
    CkAssert(msg->counter.load() == msg->test_num);
    int myExpectation = msg->test_num * num_elements;
    contribute(sizeof(int), &myExpectation, CkReduction::sum_int, msg->cb);
  }
};

class TestGroup : public CBase_TestGroup {
public:
  TestGroup() {}

  // Marked [nokeep], so I share this message with every PE on my node, and
  // should not delete it.
  void recv(TestMessage* msg) {
    for (int i = 0; i < DATA_SIZE; i++) {
      if (msg->data[i] != msg->test_num) {
        CkAbort("Bad msg->data on element %i for [nokeep] msg: %i[%i] != %i\n",
            thisIndex, msg->test_num, i, msg->data[i]);
      }
    }

    int val = msg->counter.fetch_add(1);
    contribute(sizeof(int), &val, CkReduction::sum_int, msg->cb);
  }

  // Not marked [nokeep], so I get my own copy of the message and I'm in charge
  // of deallocation.
  void recvCopy(TestMessage* msg) {
    for (int i = 0; i < DATA_SIZE; i++) {
      if (msg->data[i] != msg->test_num) {
        CkAbort("Bad msg->data on element %i for copy msg: %i[%i] != %i\n",
            thisIndex, msg->test_num, i, msg->data[i]);
      }
    }

    int val = msg->counter.fetch_add(1);
    // The value of what I received should always be the message test_num since
    // I get my own copy of the message.
    CkAssert(val == msg->test_num);

    contribute(sizeof(int), &val, CkReduction::sum_int, msg->cb);
    delete msg;
  }
};

class TestArray : public CBase_TestArray {
public:
  TestArray() {
    ngProxy.ckLocalBranch()->addElement();
  }

  void recv(TestMessage* msg) {
    for (int i = 0; i < DATA_SIZE; i++) {
      if (msg->data[i] != msg->test_num) {
        CkAbort("Bad msg->data on element %i for [nokeep] msg: %i[%i] != %i\n",
            thisIndex, msg->test_num, i, msg->data[i]);
      }
    }

    int val = msg->counter.fetch_add(1);
    contribute(sizeof(int), &val, CkReduction::sum_int, msg->cb);
  }

  void recvCopy(TestMessage* msg) {
    for (int i = 0; i < DATA_SIZE; i++) {
      if (msg->data[i] != msg->test_num) {
        CkAbort("Bad msg->data on element %i for copy msg: %i[%i] != %i\n",
            thisIndex, msg->test_num, i, msg->data[i]);
      }
    }

    int val = msg->counter.fetch_add(1);
    // The value of what I received should always be the message test_num since
    // I get my own copy of the message.
    CkAssert(val == msg->test_num);

    contribute(sizeof(int), &val, CkReduction::sum_int, msg->cb);
    delete msg;
  }
};

#include "within_node_bcast.def.h"
