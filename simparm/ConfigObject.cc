#include "Object.hh"
#include <algorithm>
#include <numeric>
#include <functional>
#include <cassert>

#include <map>
#include "stl_helpers.hh"

using namespace std;

namespace simparm {

#ifndef NDEBUG
extern void announce_dead_node( Node *node, std::string );
#endif

Object::Object(string name, string desc)
: desc("desc", desc),
  viewable("viewable", true),
  userLevel("userLevel", Beginner),
  name(name)
{
}

Object::Object(const Object& o)
: desc(o.desc), viewable(o.viewable),
  userLevel(o.userLevel), name(o.name)
{}

Object::~Object() { 
}

NodeRef Object::attach_ui( simparm::Node& node ) {
    NodeRef r = create_hidden_node( node );
    r.show();
    return r;
}

void Object::detach_ui( simparm::Node& node ) {
    node_.reset();
}

NodeRef Object::create_hidden_node( simparm::Node& node ) { 
    node_ = make_naked_node( node );
    node_->add_attribute(desc);
    node_->add_attribute(viewable);
    node_->add_attribute(userLevel);
    return *node_;
}

std::auto_ptr<Node> Object::make_naked_node( simparm::Node& node ) {
    return node.create_object( name );
}

std::string Object::getName() const { return name; }
std::string Object::getDesc() const { return desc(); }

};
