#ifndef SETTEXTCOMMAND_H
#define SETTEXTCOMMAND_H

#include <QUndoCommand>

class SetTextCommand : public QUndoCommand
{
public:
    SetTextCommand();

    void undo() override;
    void redo() override;
};

#endif // SETTEXTCOMMAND_H
