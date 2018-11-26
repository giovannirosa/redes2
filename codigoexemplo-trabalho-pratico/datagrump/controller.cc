#include <iostream>
#include <limits.h>
#include <list>
#include <iterator>
#include <math.h>

#include "controller.hh"
#include "timestamp.hh"

using namespace std;

unsigned const int MINIMUN_WINDOW_SIZE = 5;
unsigned const int TIMEOUT = 70;
float DECREASE_FACTOR = 0.7;
int decreaseCounter = 0;
int INCREASE_FACTOR = 1;
int increaseCounter = 0;
unsigned int windowSize = MINIMUN_WINDOW_SIZE;
unsigned int counter = 0;

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
  // Counter to make sure all packets are sent with specified window
  ++counter;
  if (rtt > timeout_ms() && counter >= windowSize) {
    counter = 0;
    INCREASE_FACTOR = 1;
    // Decrease window
    // if (++decreaseCounter > 20) {
    //   DECREASE_FACTOR += 0.01;
    //   decreaseCounter = 0;
    // }
    windowSize *= DECREASE_FACTOR;
    // Make sure it's at least the minimun size
    if (windowSize < MINIMUN_WINDOW_SIZE) {
      windowSize = MINIMUN_WINDOW_SIZE;
    }
    cerr << "Window size decreased by "
    << DECREASE_FACTOR
    << ", new value = " << windowSize
    << endl;
  } else if (counter >= windowSize) {
    DECREASE_FACTOR = 0.7;
    // Increase window
    // if (++increaseCounter > 20) {
    //   ++INCREASE_FACTOR;
    //   increaseCounter = 0;
    // }
    windowSize += INCREASE_FACTOR;
  }

  if ( debug_ ) {
    cerr << "At time " << timestamp_ack_received
	 << " received ack for datagram " << sequence_number_acked
	 << " (send @ time " << send_timestamp_acked
	 << ", received @ time " << recv_timestamp_acked << " by receiver's clock)"
	 << "\nRTT = " << rtt
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
