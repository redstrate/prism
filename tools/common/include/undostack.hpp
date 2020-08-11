#pragma once

#include <string>
#include <vector>

class Command {
public:
    virtual ~Command() {}
    
    std::string stored_name;
    
    std::string get_name() {
        if(stored_name.empty())
            stored_name = fetch_name();
        
        return stored_name;
    }
    
    virtual std::string fetch_name() = 0;
    
    virtual void undo() = 0;
    virtual void execute() = 0;
};

class UndoStack {
public:
    template<class T>
    T& new_command() {
        // if we are in the middle of the stack currently, wipe out everything and start fresh
        // TODO: only do it up to the point we're modifing
        if(stack_position != (command_stack.size() - 1)) {
            command_stack.clear();
            stack_position = -1;
        }
    
        stack_position++;
        
        return static_cast<T&>(*command_stack.emplace_back(std::make_unique<T>()));
    }
    
    //void push_command(Command* command);
    
    void undo();
    void redo();
    
    // useful for undo
    Command* get_last_command();
    
    // useful for redo
    Command* get_next_command();
    
    std::vector<std::unique_ptr<Command>> command_stack;
    int stack_position = -1;
};
