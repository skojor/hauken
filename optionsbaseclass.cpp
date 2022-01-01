#include "optionsbaseclass.h"

OptionsBaseClass::OptionsBaseClass()
{
    setMaximumWidth(600);
    dialog->setLayout(mainLayout);
    btnBox = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel);
}
