#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <json/json.h>
#include <QAbstractNativeEventFilter>
#include <QSystemTrayIcon>
#include "settings/settings.h"
#include <boost/algorithm/string.hpp>
namespace Ui {
	class MainWindow;
}

#include <QWebSocket>
#ifdef _WIN32
#include <Windows.h>
BOOL CALLBACK EnumWindowCallBack(HWND hwnd, LPARAM lParam);
#endif
#include "utility.h"
#include "websocketserver.h"
#include "copydatawidget.h"
#include "settings_impl.h"

extern QOverlaySettingsServer settingServer;


class MainWindow : public QMainWindow, public QAbstractNativeEventFilter, public MsgProc, public Sendable
{
	Q_OBJECT
protected:
	bool setOnly;
	bool setOnlyGlobal;

	CopyDataWidget* cdw;
	WebSocketServer* server;
	QTimer* timer;
	QMenu* menu;
	QAction* managerAction;
	QAction* exitAction;
	QSystemTrayIcon* icon;
public:
	static bool prevWindowHideFilter;
	static bool windowHideFilter;

	explicit MainWindow(uint16_t server_port, QWidget *parent = 0);
	virtual ~MainWindow();

	virtual void SendTo(ClientID sender, int code, const QString & data);
	virtual void Broadcast(int code, const QString & data);
	virtual void HandleData(ClientID sender, int code, const QString & data);
public:


	void UpdateSetting(OverlaySettings* set, const boost::uuids::uuid& id);
	void NotifySetting(OverlaySettings* set, const boost::uuids::uuid& id);
	void NotifySetting();
	void ResetPosition(OverlaySettings* set, const boost::uuids::uuid& id);
	void InitValue();
	bool DragModifiers();
	virtual void closeEvent(QCloseEvent * event);
	virtual bool eventFilter(QObject *object, QEvent *event);
	virtual bool nativeEventFilter(const QByteArray& eventType, void* message, long*);

	void SetURLList(Json::Value& value);
	void saveSettings();
	void loadSettings();
	void changeEvent(QEvent *event);
	void showEvent(QShowEvent *event);
	void hideEvent(QHideEvent *event);
public slots:
	void showManager();
	void exitManager();
	void slotActivated(QSystemTrayIcon::ActivationReason reason);
private slots:
	void on_timer();
    void on_draggable_all_shortcut_keySequenceChanged(const QKeySequence &keySequence);
    void on_no_focus_all_shortcut_keySequenceChanged(const QKeySequence &keySequence);
    void on_click_through_all_shortcut_keySequenceChanged(const QKeySequence &keySequence);
    void on_show_all_shortcut_keySequenceChanged(const QKeySequence &keySequence);
    void on_manager_shortcut_keySequenceChanged(const QKeySequence &keySequence);
    void on_newURL_returnPressed();
    void on_urlListWidget_doubleClicked(const QModelIndex &index);
	void on_listWidget_currentRowChanged(int currentRow);
    void on_opacity_valueChanged(int value);
    void on_zoom_valueChanged(int value);
    void on_fps_valueChanged(int value);
    void on_opacity_spin_valueChanged(double arg1);
    void on_zoom_spin_valueChanged(double arg1);
    void on_fps_spin_valueChanged(int arg1);
    void on_resizeable_toggled(bool checked);
    void on_draggable_toggled(bool checked);
    void on_show_toggled(bool checked);
    void on_click_through_toggled(bool checked);
    void on_no_focus_toggled(bool checked);
    void on_opacity_sliderReleased();
    void on_zoom_sliderReleased();
    void on_zoom_sliderPressed();
    void on_closeButton_clicked();
    void on_addButton_clicked();
    void on_manager_shortcut_editingFinished();
    void on_show_all_shortcut_editingFinished();
    void on_click_through_all_shortcut_editingFinished();
    void on_no_focus_all_shortcut_editingFinished();
    void on_draggable_all_shortcut_editingFinished();
    void on_click_through_all_toggled(bool checked);
    void on_no_focus_all_toggled(bool checked);
    void on_show_all_toggled(bool checked);
    void on_resizeable_all_toggled(bool checked);
    void on_draggable_all_toggled(bool checked);
    void on_dragCtrl_toggled(bool checked);
    void on_dragAlt_toggled(bool checked);
    void on_dragShift_toggled(bool checked);
	void on_appregion_toggled(bool checked);
	//void on_x_editingFinished();
    //void on_width_editingFinished();
    //void on_y_editingFinished();
    //void on_height_editingFinished();
    void on_processAddButton_clicked();
    void on_processRemoveButton_clicked();
    void on_auto_hide_clicked();
    void on_auto_hide_toggled(bool checked);
    void on_hide_filter_condition_currentIndexChanged(int index);
    void on_colorEdit_textChanged(const QString &arg1);
    //void on_backgroundResetButton_clicked();
    void on_backgroundTransparentButton_clicked();
    void on_backgroundWhiteButton_clicked();
    void on_renderer_currentIndexChanged(int index);
    void on_positionResetButton_clicked();

    void on_colorPickerButton_clicked();

    void on_styleApplyButton_clicked();

    void on_backgroundTabWidget_currentChanged(int index);

    void on_apply_image_button_clicked();

private:
	Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
