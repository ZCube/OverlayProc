#include "changerenderer.h"
#include "ui_changerenderer.h"
#include <boost/filesystem.hpp>

static boost::filesystem::path setting = "overlay_proc.json";

int main(int argc, char**argv)
{
    QApplication app(argc, argv);
	ChangeRenderer* renderer = new ChangeRenderer();
	renderer->show();
    return app.exec();
}

ChangeRenderer::ChangeRenderer(QWidget* parent)
	: QWidget(parent)
	,ui(new Ui::ChangeRenderer)
{
	ui->setupUi(this);
	if (boost::filesystem::exists(setting))
	{
		Json::Reader reader;
		Json::Value zero(0);
#ifdef _WIN32
		std::ifstream fin(setting.wstring().c_str());
#else
		std::ifstream fin(setting.string().c_str());
#endif
		if (reader.parse(fin, value))
		{
			ui->renderer->setCurrentIndex(value.get("renderer", zero).asInt());
		}
		fin.close();
	}
}
ChangeRenderer::~ChangeRenderer() {

}
void ChangeRenderer::on_buttonOK_clicked()
{
	value["renderer"] = ui->renderer->currentIndex();

	try
	{
#ifdef _WIN32
		std::ofstream fout(setting.wstring().c_str());
#else
		std::ofstream fout(setting.string().c_str());
#endif
		Json::StyledStreamWriter writer;
		writer.write(fout, value);
		fout.close();

		QApplication::instance()->quit();
	}
	catch (std::exception& e)
	{
	}

}

void ChangeRenderer::on_buttonCancel_clicked()
{
	QApplication::instance()->quit();
}
