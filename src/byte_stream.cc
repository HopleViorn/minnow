#include "byte_stream.hh"
#include <iostream>

using namespace std;

ByteStream::ByteStream( uint64_t capacity ) : capacity_( capacity ) {}

bool Writer::is_closed() const
{
  return is_closed_;
}

void Writer::push( string data )
{
  if ( is_closed() )
    return;
  if ( data.size() > available_capacity() )
    data.resize( available_capacity() );
  if ( !data.empty() ) {
    num_bytes_pushed_ += data.size();
    num_bytes_buffered_ += data.size();
    buffer.push_back( move( data ) );
  }
  if ( view_wnd_.empty() && !buffer.empty() )
    view_wnd_ = buffer.front();
}

void Writer::close()
{
  if ( !is_closed_ ) {
    is_closed_ = true;
    buffer.push_back( string( 1, EOF ) );
  }
}

uint64_t Writer::available_capacity() const
{
  return capacity_ - num_bytes_buffered_;
}

uint64_t Writer::bytes_pushed() const
{
  return num_bytes_pushed_;
}

bool Reader::is_finished() const
{
  return is_closed_ && bytes_buffered() == 0;
}

uint64_t Reader::bytes_popped() const
{
  return num_bytes_popped_;
}

string_view Reader::peek() const
{
  return view_wnd_;
}

void Reader::pop( uint64_t len )
{
  auto remainder = len;
  while ( remainder >= view_wnd_.size() && remainder != 0 ) {
    remainder -= view_wnd_.size();
    buffer.pop_front();
    view_wnd_ = buffer.empty() ? ""sv : buffer.front();
  }
  if ( !view_wnd_.empty() )
    view_wnd_.remove_prefix( remainder );

  num_bytes_buffered_ -= len;
  num_bytes_popped_ += len;
}

uint64_t Reader::bytes_buffered() const
{
  return num_bytes_buffered_;
}
