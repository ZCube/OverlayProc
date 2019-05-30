#pragma once
#include <QWidget>
#include <json/json.h>

namespace Ui {
    class ChangeRenderer;
}


class ChangeRenderer : public QWidget
{
    Q_OBJECT
protected:
public:
	ChangeRenderer(QWidget* parent = nullptr);
	virtual ~ChangeRenderer();
private slots:
    void on_buttonOK_clicked();
    void on_buttonCancel_clicked();

private:
	Json::Value value;
	Ui::ChangeRenderer *ui;
};
