#include <iostream>

#include "controller.hh"
#include "timestamp.hh"

using namespace std;
unsigned int the_window_size = 10;
unsigned const int MINIMUN_WINDOW_SIZE = 10;

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
	 << " window size is " << the_window_size << endl;
  }

  return the_window_size;
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

  // if (after_timeout) {
  //   the_window_size /= 2;
  //   cerr << "Window size divided by 2"
  //   << ", new value = " << the_window_size
  //   << endl;
  // } else {
  //   the_window_size += 1;
  // }

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
  /* Default: take no action */

  uint64_t rtt = timestamp_ack_received - send_timestamp_acked;
  if (rtt > timeout_ms()) {
    the_window_size = the_window_size*0.5;
    if (the_window_size < MINIMUN_WINDOW_SIZE) {
      the_window_size = MINIMUN_WINDOW_SIZE;
    }
    cerr << "Window size divided by 2"
    << ", new value = " << the_window_size
    << endl;
  } else {
    ++the_window_size;
  }

  if ( debug_ ) {
    cerr << "At time " << timestamp_ack_received
	 << " received ack for datagram " << sequence_number_acked
	 << " (send @ time " << send_timestamp_acked
	 << ", received @ time " << recv_timestamp_acked << " by receiver's clock)"
	 << ", RTT = " << rtt
   << ", Window Size = " << the_window_size
   << endl;
  }
}

/* How long to wait (in milliseconds) if there are no acks
   before sending one more datagram */
unsigned int Controller::timeout_ms()
{
  return 1000; /* timeout of one second */
}
