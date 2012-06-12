#include "ChoiceNode.h"
#include "WindowNode.h"
#include <wx/wx.h>
#include "lambda.h"
#include "VisibilityControl.h"
#include "GroupNode.h"
#include "gui_thread.h"

namespace simparm {
namespace wx_ui {

class ChoiceWidget : public wxChoice {
    void *shown_children;
    std::multimap< void*, boost::shared_ptr<Window> > children;
    std::map< std::string, int > name_positions;
    std::map< void*, std::string > names;
    std::string current_selection;
    boost::shared_ptr< BaseAttributeHandle > value;
    boost::shared_ptr< VisibilityControl > vc;

    void GUI_changed_choice( wxCommandEvent& ) { 
        ShowChildren(); 
        int index = GetSelection();
        if ( GetSelection() != wxNOT_FOUND )
            value->set_value( names[ GetClientData(index) ] );
        else
            value->set_value( "" );
    }
    void ShowChildren() {
        int index = GetSelection();
        void *new_shown_children = ( index == wxNOT_FOUND ) ? NULL : GetClientData(index);
        if ( shown_children == new_shown_children ) return;

        for ( std::multimap< void*, boost::shared_ptr<Window> >::const_iterator i = children.begin(); i != children.end(); ++i ) {
            if ( i->first == new_shown_children )
                i->second->change_frontend_visibility( true );
            else if ( i->first == shown_children )
                i->second->change_frontend_visibility( false );
        }
        shown_children = new_shown_children;
    }

public:
    ChoiceWidget( 
        wxWindow *parent, 
        boost::shared_ptr< BaseAttributeHandle > value, 
        boost::shared_ptr< VisibilityControl > vc
    ) 
        : wxChoice( parent, wxID_ANY ),
          shown_children(NULL),
          current_selection(*value->get_value()),
          value(value),
          vc( vc )
    {
        name_positions[""] = wxNOT_FOUND;
    }

    ~ChoiceWidget() {}

    void add_choice( void* ident, const std::string& name, std::string description ) {
        int index = Append( wxString( description.c_str(), wxConvUTF8 ), ident );
        name_positions.insert( std::make_pair( name, index ) );
        names.insert( std::make_pair( ident, name ) );
        ShowChildren();
        if ( current_selection == name )
            SetSelection( index );
    }
    
    void remove_choice( void* ident, const std::string& name, std::string description ) {
        int index = FindString( wxString( description.c_str(), wxConvUTF8 ) );
        assert( index != wxNOT_FOUND );
        Delete( index );
        name_positions.erase( name );
        names.erase( ident );
        children.erase( ident );
        ShowChildren();
    }
    
    void connect( void *ident, boost::shared_ptr<Window> window ) {
        children.insert( std::make_pair(ident, window) );
        if ( ident != shown_children )
            window->change_frontend_visibility( false );
    }

    void select_choice( std::string name ) {
        SetSelection( name_positions[name] );
        ShowChildren();
    }

    DECLARE_EVENT_TABLE();
};

BEGIN_EVENT_TABLE(ChoiceWidget, wxChoice)
    EVT_CHOICE(wxID_ANY, ChoiceWidget::GUI_changed_choice)
END_EVENT_TABLE()

void ChoiceNode::initialization_finished() {
    my_line = boost::in_place( get_relayout_function() );
    create_static_text( my_line->label, description );
    run_in_GUI_thread(
        *bl::constant(my_line->contents) =
        *bl::constant(choice) =
        bl::bind( bl::new_ptr< ChoiceWidget >(), 
                  *bl::constant( InnerNode::get_parent_window() ),
                  value_handle, 
                  get_visibility_control() ) );
    attach_help( my_line->label );
    attach_help( my_line->contents );
    is_chosen = (*value_handle->get_value() == "");
    update_visibility();
    InnerNode::add_entry_line( *my_line );
}

static void add_choice(
    boost::shared_ptr<ChoiceWidget*> choice,
    void* subchoice_ident,
    std::string name,
    std::string description
) {
    (*choice)->add_choice( subchoice_ident, name, description );
}

static void remove_choice(
    boost::shared_ptr<ChoiceWidget*> choice,
    void* subchoice_ident,
    std::string name,
    std::string description
) {
    (*choice)->remove_choice( subchoice_ident, name, description );
}


template <typename RealNode>
class ChoiceNode::SubNode : public RealNode {
    boost::shared_ptr<ChoiceNode> parent;
    std::string name, description;
    bool showed_choice;

public:
    SubNode( boost::shared_ptr<ChoiceNode> parent, std::string name )
        : RealNode(parent, name), parent(parent), name(name), showed_choice(false) {}
    ~SubNode() {
        if ( showed_choice )
            run_in_GUI_thread(
                boost::bind( &remove_choice, parent->choice, this, name, description ) );
    }

    virtual void set_description( std::string d ) { RealNode::set_description(d); description = d; }
    virtual void set_visibility( bool b ) { RealNode::set_visibility(b); show_or_hide_choice();  }
    virtual void set_user_level( UserLevel u ) { RealNode::set_user_level(u); show_or_hide_choice(); }
    void initialization_finished() {
        RealNode::notify_on_visibility_change( boost::bind( &SubNode::visibility_changed, this, _1 ) );
        RealNode::initialization_finished();
        show_or_hide_choice();
    }

    void visibility_changed( bool ) { show_or_hide_choice(); }
    void show_or_hide_choice() {
        if ( RealNode::is_visible() && ! showed_choice ) {
            ++parent->choices_count;
            parent->update_visibility();
            run_in_GUI_thread(
                boost::bind( &add_choice, parent->choice, this, name, description ) );
        }
        if ( ! RealNode::is_visible() && showed_choice ) {
            --parent->choices_count;
            parent->update_visibility();
            run_in_GUI_thread(
                boost::bind( &remove_choice, parent->choice, this, name, description ) );
        }
        showed_choice = RealNode::is_visible();
    }
    void add_entry_line( LineSpecification& s ) { 
        run_in_GUI_thread(
            bl::bind( &ChoiceWidget::connect, *bl::constant( parent->choice ), this, s.label ) );
        run_in_GUI_thread(
            bl::bind( &ChoiceWidget::connect, *bl::constant( parent->choice ), this, s.contents ) );
        run_in_GUI_thread(
            bl::bind( &ChoiceWidget::connect, *bl::constant( parent->choice ), this, s.adornment ) );
        RealNode::add_entry_line(s);
    }
    void add_full_width_line( WindowSpecification& w ) {
        run_in_GUI_thread(
            bl::bind( &ChoiceWidget::connect, *bl::constant( parent->choice ), this, w.window ) );
        RealNode::add_full_width_line(w);
    }
    void bind_visibility_group( boost::shared_ptr<Window> window ) {
        run_in_GUI_thread(
            bl::bind( &ChoiceWidget::connect, *bl::constant( parent->choice ), this, window ) );
        RealNode::bind_visibility_group(window);
    }
};

void ChoiceNode::add_attribute( simparm::BaseAttribute& a ) {
    if ( a.get_name() == "value" ) {
        value_handle.reset( new BaseAttributeHandle(a, get_protocol()) );
        connection = a.notify_on_non_GUI_value_change( 
            boost::bind( &ChoiceNode::user_changed_choice, this ) );
    }
}

void ChoiceNode::user_changed_choice() {
    is_chosen = (*value_handle->get_value() == "");
    update_visibility();
    run_in_GUI_thread( 
        bl::bind( &ChoiceWidget::select_choice, *bl::constant(choice), *value_handle->get_value() ) );
}

NodeHandle ChoiceNode::create_object( std::string name ) {
    return NodeHandle( new SubNode<InnerNode>( boost::static_pointer_cast<ChoiceNode>( shared_from_this() ), name ) );
}

NodeHandle ChoiceNode::create_group( std::string name ) {
    return NodeHandle( new SubNode<GroupNode>( boost::static_pointer_cast<ChoiceNode>( shared_from_this() ), name ) );
}

void ChoiceNode::update_visibility() {
    bool should_be_visible = choices_count >= 1 || ( choices_count >= 1 && ! is_chosen );
    if ( should_be_visible != visible && my_line ) {
        my_line->label->change_frontend_visibility( should_be_visible );
        my_line->contents->change_frontend_visibility( should_be_visible );
        visible = should_be_visible;
    }
}

}
}
