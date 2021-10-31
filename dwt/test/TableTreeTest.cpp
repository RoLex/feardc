#include <dwt/widgets/Window.h>
#include <dwt/widgets/TableTree.h>
#include <dwt/resources/ImageList.h>

#include <boost/format.hpp>

using dwt::tstring;

#if 1 // make this 0 to test a redraw problem that occurs when the last parent has 1 child.

const size_t COLUMNS = 3;
const size_t ROWS = 100;

const size_t PARENT = 2;
const size_t CHILDREN = 10;

const auto IMAGE_SIZE = 32;

#else

const size_t COLUMNS = 3;
const size_t ROWS = 10;

const size_t PARENT = 9;
const size_t CHILDREN = 1;

const auto IMAGE_SIZE = 32;

#endif

struct Item {
	tstring texts[COLUMNS];
};

int dwtMain(dwt::Application& app)
{
	auto window = new dwt::Window();
	window->create();
	window->onClosing([] { return ::PostQuitMessage(0), true; });

	auto seed = dwt::TableTree::Seed(dwt::Table::Seed());
	seed.style |= WS_HSCROLL | WS_VSCROLL | LVS_SHOWSELALWAYS | LVS_SHAREIMAGELISTS;
	seed.lvStyle |= LVS_EX_HEADERDRAGDROP | LVS_EX_FULLROWSELECT | LVS_EX_LABELTIP;
	auto table = window->addChild(seed);

	table->onSortItems([table](LPARAM lhs, LPARAM rhs) -> int {
		auto col = table->getSortColumn();
		return wcscoll(reinterpret_cast<Item*>(lhs)->texts[col].c_str(), reinterpret_cast<Item*>(rhs)->texts[col].c_str());
	});
	table->onColumnClick([table](int column) {
		if(column != table->getSortColumn()) {
			table->setSort(column, dwt::Table::SORT_CALLBACK, true);
		} else if(table->isAscending()) {
			table->setSort(table->getSortColumn(), dwt::Table::SORT_CALLBACK, false);
		} else {
			table->setSort(-1, dwt::Table::SORT_CALLBACK, true);
		}
	});
	table->setSort(0, dwt::Table::SORT_CALLBACK);

	table->onKeyDown([table](int c) -> bool {
		if(c == VK_DELETE) {
			int i = -1;
			while((i = table->getNext(-1, LVNI_SELECTED)) != -1) {
				table->erase(i);
			}
			return true;
		}
		return false;
	});

	dwt::ImageListPtr images(new dwt::ImageList(dwt::Point(IMAGE_SIZE, IMAGE_SIZE)));
	images->add(dwt::Icon(::LoadIcon(nullptr, IDI_INFORMATION), false));
	table->setSmallImageList(images);

	for(size_t i = 0; i < COLUMNS; ++i) {
		table->addColumn(dwt::Column((boost::wformat(L"Column %d") % i).str()));
		table->setColumnWidth(i, 200);
	}

	std::vector<Item> items(ROWS);

	for(size_t i = 0; i < items.size(); ++i) {
		for(size_t j = 0; j < COLUMNS; ++j) {
			items[i].texts[j] = (boost::wformat(L"Item %d in col %d") % i % j).str();
		}
	}

	table->onRaw([](WPARAM, LPARAM lParam) -> LRESULT {
		auto& data = *reinterpret_cast<NMLVDISPINFO*>(lParam);
		if(data.item.mask & LVIF_TEXT) {
			const auto& text = reinterpret_cast<Item*>(data.item.lParam)->texts[data.item.iSubItem];
			_tcsncpy(data.item.pszText, text.data(), text.size());
			data.item.pszText[text.size()] = 0;
		}
		if(data.item.mask & LVIF_IMAGE) {
			data.item.iImage = 0;
		}
		return 0;
	}, dwt::Message(WM_NOTIFY, LVN_GETDISPINFO));

	for(auto& item: items) {
		table->insert(LVIF_TEXT | LVIF_PARAM | LVIF_IMAGE, table->size(),
			LPSTR_TEXTCALLBACK, 0, 0, I_IMAGECALLBACK, reinterpret_cast<LPARAM>(&item));
	}

	std::vector<Item> children(CHILDREN);

	for(size_t i = 0; i < children.size(); ++i) {
		for(size_t j = 0; j < COLUMNS; ++j) {
			children[i].texts[j] = (boost::wformat(L"Child %d in col %d") % i % j).str();
		}
	}

	auto parent = reinterpret_cast<LPARAM>(&items[PARENT]);
	for(auto& child: children) {
		table->insertChild(parent, reinterpret_cast<LPARAM>(&child));
	}
	table->expand(parent);

	table->resize(window->getClientSize());
	window->onSized([=](const dwt::SizedEvent&) { table->resize(window->getClientSize()); });

	app.run();

	return 0;
}
