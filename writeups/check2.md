Checkpoint 2 Writeup
====================

My name: 卢郡然

My Student ID: 502024330034

This lab took me about 4 hours to do 

## Implementation of Wrap32

#### Program Structure and Design

`wrap` method is easy to implement. just treat the value it wraps as values in $\mod 2^{32}$ congruence system. Since there is only addition here, we just need to do a modulo $2^{32}$after each addition. 

Calculate the offset by :
$$
\text{wrap_result} = (n +\text{zero_point})\mod 2^{32}
$$
`unwrap` is the same. The values are also in the congruence system. We handle it by simply subtracting and get $base$.

The only problem is that we need to find an absolution value that closet to checkpoint. That is:
$$
\min_{k}|base +k*2^{32}-checkpoint|
$$
Assume that base and checkpoint are positive. The lower bound should be :
$$
low = base + \lfloor\frac{checkpoint}{2^{32}}\rfloor
$$
The upper bound is:
$$
high = base + \lceil\frac{checkpoint}{2^{32}}\rceil
$$
It is obvious that the minimum is one of this two bound values. we can easily get the answer by calculating:
$$
\min (checkpoint - low, high - checkpoint)
$$

## Implementation of Receive and Send

#### Program Structure and Design

The `receive` method is to receive TCP segments and reassemble them. Just implemented it by getting the payload and being careful with some corner cases.

Keep an `optional<Wrap>` as a mark of whether a TCP connection is established and store the ISN meanwhile. 

The index of payload sent to reassembler is differ by 1 from seqno. Just unwrap the seqno and subtract it by 1 and send it to reassemble.

The `send` method constructs a `TCPReceiverMessage` that indicates the receiver's current state, including the acknowledgment number and available window size.

Compute the acknowledgment number (`ACK`) as the number of bytes successfully written by the reassembler, plus 1 if the ISN has been set.

The window size represents the available capacity in the reassembler for receiving more data, capped at 65535 bytes, as per TCP protocol limits.

The ACK message is sent with information above with error makrs.