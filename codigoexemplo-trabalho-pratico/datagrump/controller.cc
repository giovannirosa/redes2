#include <iostream>
#include <limits.h>

#include "controller.hh"
#include "timestamp.hh"

using namespace std;


unsigned int rcv_window_size = INT_MAX;
unsigned int ssthresh = INT_MAX;
unsigned int rto = 200;

unsigned const int MINIMUN_WINDOW_SIZE = 5;
unsigned const int TIMEOUT = 70;
const float DECREASE_FACTOR = 0.7;
const int INCREASE_FACTOR = 1;
unsigned int windowSize = MINIMUN_WINDOW_SIZE;
bool divided = false;
unsigned int counter = 0;
unsigned int last_window_size = 0;
uint64_t actual_seq;

/* Default constructor */
Controller::Controller( const bool debug )
  : debug_( debug )
{}

/* Get current window size, in datagrams */
unsigned int Controller::window_size()
{
  /* Default: fixed window size of 100 outstanding datagrams */
  

  if ( debug_ ) {
    cerr << "At time " << timestamp_ms()
	 << " window size is " << windowSize << endl;
  }

  return windowSize;
}

/* A datagram was sent */
void Controller::datagram_was_sent( const uint64_t sequence_number,
				    /* of the sent datagram */
				    const uint64_t send_timestamp,
                                    /* in milliseconds */
				    const bool after_timeout
				    /* datagram was sent because of a timeout */ )
{
  /* Default: take no action */

  if (after_timeout) {
    windowSize = 0;
    cerr << "Window size divided by 2"
    << ", new value = " << windowSize
    << endl;
  }

  if ( debug_ ) {
    cerr << "At time " << send_timestamp
	 << " sent datagram " << sequence_number << " (timeout = " << after_timeout << ")\n";
  }
}

int increaseMode = 1;

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
  // if (sequence_number_acked > actual_seq)
  //   actual_seq = sequence_number_acked;
  // else
  //   return;
  

  // if (windowSize >= ssthresh) { // Congestion Avoidance phase
  //     windowSize += 1/windowSize;
  //   } else {
  //     ++windowSize;
  //   }
  
  // if (rtt > rto) {
  //   windowSize *= 0.5;
  // //   if (windowSize < 2)
  // //     ssthresh = 2;
  // //   else
  // //     ssthresh = windowSize;
  // //   windowSize = 1;
  // }

  uint64_t rtt = timestamp_ack_received - send_timestamp_acked;
  ++counter;
  if (rtt > timeout_ms() && counter >= windowSize) {
    counter = 0;
    windowSize *= DECREASE_FACTOR;
    if (windowSize < MINIMUN_WINDOW_SIZE) {
      windowSize = MINIMUN_WINDOW_SIZE;
    }
    cerr << "Window size decreased by "
    << DECREASE_FACTOR
    << ", new value = " << windowSize
    << endl;
  } else if (counter >= windowSize) {
    windowSize += INCREASE_FACTOR;
  }





  /* Default: take no action */
  // if (sequence_number_acked > actual_seq)
  //   actual_seq = sequence_number_acked;

  // if (rtt > timeout_ms()) {
  //   ++counter;
  // }
  // if (rtt > timeout_ms() && counter > 500) {
  //   last_window_size = windowSize*0.6;
  //   windowSize = windowSize*0.5;
  //   if (windowSize < MINIMUN_WINDOW_SIZE) {
  //     windowSize = MINIMUN_WINDOW_SIZE;
  //   }
  //   // divided = true;
  //   counter = 0;
  //   increaseMode = 0;
  //   cerr << "Window size divided by 2"
  //   << ", new value = " << windowSize
  //   << endl;
  // } else if(rtt <= timeout_ms()) {
  //   if (increaseMode) {
  //     windowSize *= 2;
  //   } else {
  //     ++windowSize;
  //   }
  //   // divided = false;
  // }

  if ( debug_ ) {
    cerr << "At time " << timestamp_ack_received
	 << " received ack for datagram " << sequence_number_acked
	 << " (send @ time " << send_timestamp_acked
	 << ", received @ time " << recv_timestamp_acked << " by receiver's clock)"
	 << ", RTT = " << rtt
   << ", Window Size = " << windowSize
   << ", Counter = " << counter
   << endl;
  }
}

/* How long to wait (in milliseconds) if there are no acks
   before sending one more datagram */
unsigned int Controller::timeout_ms()
{
  return TIMEOUT;
}
