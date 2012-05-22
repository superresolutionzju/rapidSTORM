#ifndef SIMPARM_NODE
#define SIMPARM_NODE

#if 0
#include <list>
#include <stdexcept>
#include <memory>

#include "Link.hh"
#include "Link_iterator.hh"

#include "Callback.hh"

#include <iostream>
#endif

#include <string>
#include <memory>
#include "Message_decl.hh"
#include "BaseAttribute.hh"

namespace simparm {

#if 0
class BasicEntry;

class Node : public Publisher {
  private:
    typedef Link<Node,Node> TreeLink;
    friend class Link<Node,Node>;
    /** Callback function for simparm::Link */
    template <class From, class To, bool at_up_node>
        typename Link<From,To>::List& get_link_list();
    /** Callback function for simparm::Link */
    void link_removed( TreeLink::WhichEnd end, Node& other_end ); 

  public:
    typedef Listener Callback;

    typedef TreeLink::down_iterator iterator;
    typedef TreeLink::const_down_iterator const_iterator;

    typedef TreeLink::up_iterator parents_iterator;
    typedef TreeLink::const_up_iterator const_parents_iterator;

  private:
    /* Encapsulate private data to reduce number of needed inclusions. */
    struct Pimpl;
    Pimpl* data;

    iterator insert_( const iterator& where, Node& object );

  protected:
    const std::string name;

    virtual std::string getTypeDescriptor() const = 0;

    virtual void print(const std::string& what);

  public:
    static bool is_valid_name(const std::string& name);
    Node(const std::string& name);
    Node(const Node& o);
    virtual ~Node();
    virtual Node *clone() const = 0;
    Node& operator=(const Node&o);

    const Node& getNode() const { return *this; }
    Node& getNode() { return *this; }
    const std::string& getName() const { return name; }

    void push_front(Node& object) { insert_( begin(), object ); }
    void push_back(Node& object) { insert_( end(), object ); }
    iterator insert( const iterator& where, Node& object )
        { return insert_( where, object ); }

    virtual void send( Message& m );

    iterator erase( const iterator& );
    /** @throw invalid_argument When the given Node is 
     *         not amongst the direct children of this Node. */
    void erase(Node& object);
    /** @throw invalid_argument when no Node with the given name is 
     *         found amongst the direct children. */
    void erase(const std::string& name);

    void clearChildren();
    void clearParents();

    bool isActive() const;
    void setActivity(bool active);

    virtual std::string define();
    virtual std::string undefine();
    virtual std::list<std::string> printValues() const;
    virtual void printHelp(std::ostream &) const {}
    virtual void processCommand(std::istream& from);
    virtual bool has_active_choice() const { return false; }
    virtual const Node& get_active_choice() const { throw std::logic_error("Not implemented"); }

    const Node& operator[](const std::string& name) const;
    Node& operator[](const std::string& name);
    bool has_child_named(const std::string& name) const;

    int size() const;

    const_iterator begin() const; 
    const_iterator end() const;
    iterator begin();
    iterator end();

    const_parents_iterator begin_parents() const; 
    const_parents_iterator end_parents() const;
    parents_iterator begin_parents();
    parents_iterator end_parents();

    void make_thread_safe();
    bool is_thread_safe();
    void remove_thread_safety();

    virtual bool is_entry() const { return false; }
    virtual BasicEntry& get_entry() { throw std::logic_error("Not implemented"); }
};
#endif

struct Node {
    virtual ~Node() {}
    virtual std::auto_ptr<Node> create_object( std::string name ) = 0;
    virtual std::auto_ptr<Node> create_entry( std::string name, std::string desc, std::string type ) = 0;
    virtual std::auto_ptr<Node> create_set( std::string name ) = 0;
    virtual std::auto_ptr<Node> create_choice( std::string name, std::string desc ) = 0;
    virtual std::auto_ptr<Node> create_file_entry( std::string name, std::string desc ) = 0;
    virtual std::auto_ptr<Node> create_progress_bar( std::string name, std::string desc ) = 0;
    virtual std::auto_ptr<Node> create_trigger( std::string name, std::string desc ) = 0;
    virtual void add_attribute( simparm::BaseAttribute& ) = 0;
    virtual void send( Message& m ) = 0;
    virtual void show() = 0;
    /** TODO: Method is deprecated and should be removed on successful migration. */
    virtual bool isActive() const = 0;
};

typedef Node& NodeRef;

}

#endif
