#include <iostream>
#include <limits.h>
#include <list>
#include <iterator>
#include <math.h>

#include "controller.hh"
#include "timestamp.hh"

using namespace std;

typedef struct rttStore rttStore;

struct rttStore {
  uint64_t rtt;
  uint64_t ackTime;
};

// SRTT = (1 - alpha) * SRTT + alpha * R'
const double ALPHA = 0.125;
// const double BETA = 0.25;
const double GAMA = 0.5;
double SRTT = 0; // current value of the standard smoothed RTT estimate
uint64_t recentTimeWindow = 0; // tau = srtt / 2
rttStore *RTTstanding; // smallest RTT observed over a recent time window, tau
double currentRate = 0; // delta = windowSize / RTTstanding
double targetRate = 0; // delta = 1 / gama * queueDelay
rttStore *RTTmin; // smallest RTT observed over a long period of time (10s - start)
uint64_t queueDelay = 0; // d = RTTstanding - RTTmin
int v = 1; // velocity parameter
int direction = 1; // 1 = UP and 0 = DOWN
int dirCounter = 0;
int lastDirection = 1; // 1 = UP and 0 = DOWN
bool slowStart = true;

unsigned const int MINIMUN_WINDOW_SIZE = 1;
unsigned const int TIMEOUT = 60;
double windowSize = MINIMUN_WINDOW_SIZE;
unsigned int lastWindowSize = 0;
unsigned int counter = 0;

list<rttStore*> rttStoreList;

/* Default constructor */
Controller::Controller( const bool debug )
  : debug_( debug )
{
  RTTmin = (rttStore*) malloc(sizeof(rttStore));
  RTTmin->rtt = INT_MAX;
  RTTstanding = (rttStore*) malloc(sizeof(rttStore));
  RTTstanding->rtt = INT_MAX;
}

/* Get current window size, in datagrams */
unsigned int Controller::window_size()
{
  /* Default: fixed window size of 100 outstanding datagrams */
  

  if ( debug_ ) {
    cerr << "At time " << timestamp_ms()
	 << " window size is " << windowSize << endl;
  }

  return (int)windowSize;
}

/* A datagram was sent */
void Controller::datagram_was_sent( const uint64_t sequence_number,
				    /* of the sent datagram */
				    const uint64_t send_timestamp,
                                    /* in milliseconds */
				    const bool after_timeout
				    /* datagram was sent because of a timeout */ )
{
  // Close the window until timeout is solved
  if (after_timeout) {
    windowSize = 0;
    slowStart = true;
    cerr << "Window size down to 0"
    << endl;
  }

  if ( debug_ ) {
    cerr << "At time " << send_timestamp
	 << " sent datagram " << sequence_number << " (timeout = " << after_timeout << ")\n";
  }
}

/* An ack was received */
void Controller::ack_received( const uint64_t sequence_number_acked,
			       /* what sequence number was acknowledged */
			       const uint64_t send_timestamp_acked,
			       /* when the acknowledged datagram was sent (sender's clock) */
			       const uint64_t recv_timestamp_acked,
			       /* when the acknowledged datagram was received (receiver's clock)*/
			       const uint64_t timestamp_ack_received )
                               /* when the ack was received (by sender) */
{
  // RTT is the difference between send time and ack time
  uint64_t rtt = timestamp_ack_received - send_timestamp_acked;
  if (SRTT == 0) {
    // Initialize SRTT
    SRTT = rtt;
  } else {
    // SRTT = (1 - alpha) * SRTT + alpha * R'
    SRTT = (1 - ALPHA) * SRTT + ALPHA * rtt;
  }

  // tau = srtt / 2
  recentTimeWindow = SRTT * 0.5;

  if ((timestamp_ack_received - RTTmin->ackTime) > 10000 || rtt < RTTmin->rtt) {
    RTTmin->rtt = rtt;
    RTTmin->ackTime = timestamp_ack_received;
  }

  if ((timestamp_ack_received - RTTstanding->ackTime) > recentTimeWindow || rtt < RTTstanding->rtt) {
    RTTstanding->rtt = rtt;
    RTTstanding->ackTime = timestamp_ack_received;
  }

  if (++counter >= windowSize) {
    if (windowSize == 0) {
      windowSize = MINIMUN_WINDOW_SIZE;
    }

    queueDelay = RTTstanding->rtt - RTTmin->rtt;
    currentRate = (double)windowSize / RTTstanding->rtt;
    targetRate = 1 / (GAMA * queueDelay);

    if (currentRate <= targetRate) {
      if (slowStart) {
        windowSize *= 2;
      } else {
        windowSize += (double)v / (GAMA * windowSize);
      }
    } else {
      slowStart = false;
      windowSize -= (double)v / (GAMA * windowSize);
    }
    counter = 0;

    // Velocity parameter calculation (once per window)
    if (windowSize > lastWindowSize) {
      direction = 1;
    } else {
      direction = 0;
    }

    if (direction == lastDirection) {
      if (++dirCounter >= 3) {
        if (v > 8) {
          v = 2;
        } else {
          v *= 2;
        }
        dirCounter = 0;
      }
    } else {
      v = 1;
    }
    lastWindowSize = windowSize;
    lastDirection = direction;
  }

  if ( debug_ ) {
    cerr << "At time " << timestamp_ack_received
	 << " received ack for datagram " << sequence_number_acked
	 << " (send @ time " << send_timestamp_acked
	 << ", received @ time " << recv_timestamp_acked << " by receiver's clock)"
	 << "\nRTT = " << rtt
   << ", Window Size = " << windowSize
   << ", Counter = " << counter
   << ", Recent Time Window = " << recentTimeWindow
   << ", RTTstanding = " << RTTstanding->rtt
   << ", RTTmin = " << RTTmin->rtt
   << ", currentRate = " << currentRate
   << ", targetRate = " << targetRate
   << ", queueDelay = " << queueDelay
   << ", velocity = " << v
   << ", direction = " << (direction == 0 ? "DOWN" : "UP")
   << endl;
  }
}

/* How long to wait (in milliseconds) if there are no acks
   before sending one more datagram */
unsigned int Controller::timeout_ms()
{
  return TIMEOUT;
}
