#pragma once
#include <QKeySequenceEdit>
#include <QEvent>
#include <QKeyEvent>
#include <QPushButton>
#include <QLayout>

inline int translateModifiers(Qt::KeyboardModifiers state,
	const QString &text)
{
	int result = 0;
	// The shift modifier only counts when it is not used to type a symbol
	// that is only reachable using the shift key anyway
	if ((state & Qt::ShiftModifier) && (text.size() == 0
		|| !text.at(0).isPrint()
		|| text.at(0).isLetterOrNumber()
		|| text.at(0).isSpace()))
		result |= Qt::SHIFT;
	if (state & Qt::ControlModifier)
		result |= Qt::CTRL;
	if (state & Qt::MetaModifier)
		result |= Qt::META;
	if (state & Qt::AltModifier)
		result |= Qt::ALT;
	return result;
}

class QOneKeySequenceEdit : public QKeySequenceEdit
{
	Q_OBJECT
public:
	QOneKeySequenceEdit(QWidget* parent = nullptr) :
		QKeySequenceEdit(parent)
	{
		QPushButton* clear = new QPushButton();
		clear->setText(tr("Clear"));
		this->layout()->addWidget(clear);

		connect(clear, SIGNAL(clicked()), this,  SLOT(on_clear_buttonPressed()));
	}
protected:
	void keyPressEvent(QKeyEvent *e)
	{
		setKeySequence(QKeySequence());
	    int nextKey = e->key();

		if (nextKey == Qt::Key_Control
			|| nextKey == Qt::Key_Shift
			|| nextKey == Qt::Key_Meta
			|| nextKey == Qt::Key_Alt) {
			return;
		}

		if (e->modifiers() == Qt::NoModifier)
		{
			return;
		}


		QKeySequenceEdit::keyPressEvent(e);
	}

	private slots:
	void on_clear_buttonPressed()
	{
		setKeySequence(QKeySequence());
	}
};