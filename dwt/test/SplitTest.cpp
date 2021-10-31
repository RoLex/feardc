#include <dwt/widgets/Window.h>
#include <dwt/widgets/Label.h>
#include <dwt/widgets/SplitterContainer.h>

int dwtMain(dwt::Application& app)
{
	auto window = new dwt::Window();
	window->create();
	window->onClosing([] { return ::PostQuitMessage(0), true; });

	auto split = window->addChild(dwt::SplitterContainer::Seed(0.3, true));

	split->addChild(dwt::Label::Seed(_T("First row")));
	split->addChild(dwt::Label::Seed(_T("Second row")));
	split->addChild(dwt::Label::Seed(_T("Third row")));

	split->resize(window->getClientSize());
	window->onSized([=](const dwt::SizedEvent&) { split->resize(window->getClientSize()); });

	app.run();

	return 0;
}
