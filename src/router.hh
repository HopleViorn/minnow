#pragma once

#include <memory>
#include <optional>
#include <set>
#include<iostream>

#include "exception.hh"
#include "network_interface.hh"

// \brief A router that has multiple network interfaces and
// performs longest-prefix-match routing between them.

struct RouteEntry{
  uint32_t route_prefix;
  uint8_t prefix_length;
  std::optional<Address> next_hop;
  size_t interface_num;
  RouteEntry(uint32_t route_prefix_, uint8_t prefix_length_, std::optional<Address> next_hop_, size_t interface_num_): route_prefix(route_prefix_), prefix_length(prefix_length_), next_hop(next_hop_), interface_num(interface_num_){}
  bool operator<(const RouteEntry& other) const {
    return route_prefix < other.route_prefix || (route_prefix == other.route_prefix && prefix_length < other.prefix_length);
  }
};

struct TrieNode
{
  std::unique_ptr<TrieNode> children[2];
  std::optional<RouteEntry> prefix;
  TrieNode(): children{nullptr, nullptr}, prefix(std::nullopt) {}
};

class Trie
{
public:
  TrieNode root;
  
  void insert(RouteEntry prefix_order){
    TrieNode* node = &root;
    uint8_t prefix_length = prefix_order.prefix_length;
    uint32_t route_prefix = prefix_order.route_prefix;
    for (int i = 31; i >= 32 - prefix_length; i--) {
      bool bit = (route_prefix >> i) & 1;
      if (node->children[bit] == nullptr) {
        node->children[bit] = std::make_unique<TrieNode>();
      }
      node = node->children[bit].get();
    }
    node->prefix = prefix_order;
  }
  
  std::optional<RouteEntry> match(uint32_t ip_address){
    TrieNode* node = &root;
    std::optional<RouteEntry> longest_match = std::nullopt;
    for (int i = 31; i >= 0; i--) {
      if (node->prefix.has_value()) {
        longest_match = node->prefix;
      }
      bool bit = (ip_address >> i) & 1;
      if (node->children[bit] == nullptr) {
        return longest_match;
      }
      node = node->children[bit].get();
    }
    return longest_match;
  }
};

class Router
{
public:
  // Add an interface to the router
  // \param[in] interface an already-constructed network interface
  // \returns The index of the interface after it has been added to the router
  size_t add_interface( std::shared_ptr<NetworkInterface> interface )
  {
    _interfaces.push_back( notnull( "add_interface", std::move( interface ) ) );
    return _interfaces.size() - 1;
  }

  // Access an interface by index
  std::shared_ptr<NetworkInterface> interface( const size_t N ) { return _interfaces.at( N ); }


  // Add a route (a forwarding rule)
  void add_route( uint32_t route_prefix,
                  uint8_t prefix_length,
                  std::optional<Address> next_hop,
                  size_t interface_num );

  Trie _trie {};

  size_t match_and_route(InternetDatagram& datagram); 
  // Route packets between the interfaces
  void route();

private:
  // The router's collection of network interfaces
  std::vector<std::shared_ptr<NetworkInterface>> _interfaces {};
};
