#include "byte_stream.hh"
#include <iostream>
#define zxc(x) cerr<<(#x)<<'='<<(x)<<'\n'
using namespace std;

ByteStream::ByteStream( uint64_t capacity ) : capacity_( capacity ) {}

bool Writer::is_closed() const
{
  return closed_;
}

void Writer::push( string data )
{
  if ( is_closed() )
    return;
  data = data.substr(0, min(data.length(), available_capacity()));
  if ( !data.empty() ) {
    num_bytes_pushed += data.size();
    num_bytes_buffered += data.size();
    buffer.push_back( move( data ) );
  }
  if ( remain.empty() && !buffer.empty() )
    remain = buffer.front();
}

void Writer::close()
{
  zxc("CLOSE!");
  if ( !closed_ ) {
    closed_ = true;
    // buffer.push_back( string( 1, EOF ) );
  }
}

uint64_t Writer::available_capacity() const
{
  return capacity_ - num_bytes_buffered;
}

uint64_t Writer::bytes_pushed() const
{
  return num_bytes_pushed;
}

bool Reader::is_finished() const
{
  return closed_ && bytes_buffered() == 0;
}

uint64_t Reader::bytes_popped() const
{
  return num_bytes_popped;
}

string_view Reader::peek() const
{
  return remain;
}

void Reader::pop( uint64_t len )
{
  num_bytes_buffered -= len;
  num_bytes_popped += len;
  while ( len >= remain.size() && len != 0 ) {
    len -= remain.size();
    buffer.pop_front();
    remain = buffer.empty() ? ""sv : buffer.front();
  }
  if ( !remain.empty() )
    remain.remove_prefix( len );
}

uint64_t Reader::bytes_buffered() const
{
  return num_bytes_buffered;
}
