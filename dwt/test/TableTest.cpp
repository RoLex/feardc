#include <dwt/widgets/Window.h>
#include <dwt/widgets/Table.h>

using dwt::tstring;

int dwtMain(dwt::Application& app)
{
	auto window = new dwt::Window();
	window->create();
	window->onClosing([] { return ::PostQuitMessage(0), true; });

	auto seed = dwt::Table::Seed();
	seed.lvStyle |= LVS_EX_CHECKBOXES;
	auto table = window->addChild(seed);

	table->addColumn(dwt::Column(_T("Column A")));
	table->addColumn(dwt::Column(_T("Column B")));

	table->eraseColumn(1);
	table->addColumn(_T("Column C"), dwt::Column::SIZE_TO_HEADER);

	table->setColumnWidth(0, 100);
	table->setColumnWidth(1, 200);

	auto order = table->getColumnOrder();
	order[0] = 1;
	order[1] = 0;
	table->setColumnOrder(order);

	std::vector<tstring> row(2);

	row[0] = _T("A1");
	row[1] = _T("B1");
	table->insert(row);

	row[0] = _T("A2");
	row[1] = _T("B2");
	table->insert(row);

	table->resize(window->getClientSize());
	window->onSized([=](const dwt::SizedEvent&) { table->resize(window->getClientSize()); });

	app.run();

	return 0;
}
