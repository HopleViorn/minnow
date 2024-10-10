#include "reassembler.hh"
#include <algorithm>
#include <iostream>

using namespace std;


void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring ){
  Writer& writer = output_.writer();
  uint64_t limit = head_index + writer.available_capacity();
  if (data.length() == 0){
    if (is_last_substring && last_index==0x7fffffff) {
      writer.close();
    }
    return ;
  }
  if(is_last_substring){
    last_index = first_index + data.length();
  }
  if (first_index >= limit) {
    return ;
  }
  data = data.substr(0, min(data.length(),limit - first_index));

  Node node = Node(first_index, move(data));

  bool insert = false;
  if(buffer.empty()){
    num_bytes_pending += node.data.length();
    buffer.insert(node);
  }else{
    auto it = buffer.lower_bound(node);
    if(it != buffer.begin()){
      it--;
      //cover 
      if(it->index + it->data.length() >= node.index + node.data.length()){
        insert = false;
      }else if(it->index + it->data.length() <= node.index){
        insert = true;
      }else if(it->index + it->data.length() > node.index){
        node.data = node.data.substr(it->index + it->data.length() - node.index);
        node.index = it->index + it->data.length();
        insert = true;
      }else{
        exit(-1);
      }
    }else{
      insert = true;
    }
    if(insert){
        it= buffer.lower_bound(node);
        while(it != buffer.end() && it->index < node.index + node.data.length()){
          if(it->index + it->data.length() > node.index + node.data.length()){
            node.data = node.data.substr(0, it->index-node.index);
            break;
          }else{
            num_bytes_pending -= it->data.length();
            it = buffer.erase(it);
          }
        }
        if(node.data.length() > 0){
          num_bytes_pending += node.data.length();
          buffer.insert(node);
        }
      }
  }


  while(!buffer.empty() && buffer.begin()->index <= head_index){
    auto top = buffer.begin();
    num_bytes_pending -= top->data.length();
    if(top->index + top->data.length() > head_index){
      string sub = top->data.substr((head_index - top->index), top->data.length()-(head_index - top->index));
      writer.push(sub);
      head_index += sub.length();
      if(head_index >= last_index){
        writer.close();
      }
    }
    buffer.erase(top);
  }
}


uint64_t Reassembler::bytes_pending() const
{
  return num_bytes_pending;
}