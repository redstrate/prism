#include "undostack.hpp"

void UndoStack::undo() {
    if(stack_position >= 0) {
        command_stack[stack_position]->undo();
        stack_position--;
    }
}

void UndoStack::redo() {
    if((stack_position + 1) < command_stack.size()) {
        stack_position++;
        command_stack[stack_position]->execute();
    }
}

Command* UndoStack::get_last_command() {
    if(command_stack.empty())
        return nullptr;
    
    if(stack_position >= 0)
        return command_stack[stack_position].get();

    return nullptr;
}

Command* UndoStack::get_next_command() {
    if(command_stack.empty())
        return nullptr;
    
    if((stack_position + 1) < command_stack.size())
        return command_stack[stack_position + 1].get();

    return nullptr;
}
