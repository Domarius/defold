package com.dynamo.cr.contenteditor.commands;

import org.eclipse.core.commands.ExecutionEvent;
import org.eclipse.core.commands.ExecutionException;
import org.eclipse.ui.IEditorPart;
import org.eclipse.ui.handlers.HandlerUtil;
import org.eclipse.ui.handlers.RadioState;

import com.dynamo.cr.contenteditor.editors.IEditor;

public class ActivateTool extends AbstractRadioHandler {
    public static final String COMMAND_ID = "com.dynamo.cr.contenteditor.commands.activateTool";

    @Override
    public String getCommandId() {
        return COMMAND_ID;
    }

    @Override
    public Object execute(ExecutionEvent event) throws ExecutionException {
        IEditorPart editor = HandlerUtil.getActiveEditor(event);
        if (editor instanceof IEditor) {
            if(HandlerUtil.matchesRadioState(event))
                return null; // we are already in the updated state - do nothing

            String currentState = event.getParameter(RadioState.PARAMETER_ID);

            // do whatever having "currentState" implies
            ((IEditor) editor).setManipulator(currentState);

            // and finally update the current state
            HandlerUtil.updateRadioState(event.getCommand(), currentState);

        }
        return null;
    }

}
