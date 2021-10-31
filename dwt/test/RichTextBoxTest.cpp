#include <dwt/widgets/Window.h>
#include <dwt/widgets/RichTextBox.h>

#include <fstream>

#include <boost/format.hpp>

// set to "\n" & 16384 to test a bug when adding too many new lines at once.
// ref: <https://bugs.launchpad.net/dcplusplus/+bug/1681153>.
const auto REPEATED_TEXT = L"ABC\n";
const auto REPEAT_COUNT = 16384;

#define ADD_TO_RICHTEXTBOX 1
#define WRITE_TO_FILE 0

int dwtMain(dwt::Application& app)
{
	std::wstring text_per_click;
	for (auto i = 0; i < REPEAT_COUNT; ++i)
		text_per_click += REPEATED_TEXT;

	auto window = new dwt::Window();
	window->create();
	window->onClosing([] { return ::PostQuitMessage(0), true; });

	auto richtextbox = window->addChild(dwt::RichTextBox::Seed());
	richtextbox->setTextLimit(1024*64*2);

	richtextbox->setText(L"click to add text");

	auto counter = 0;

	richtextbox->onLeftMouseDown([&](const auto&) {
		const auto text = L"{\\urtf1\n" + dwt::RichTextBox::rtfEscape(
			text_per_click + (boost::wformat(L"Done #%d") % counter++).str()
		) + L"}\n";
#if ADD_TO_RICHTEXTBOX
		richtextbox->addTextSteady(text);
#endif
#if WRITE_TO_FILE
		std::wofstream("RichTextBoxTest-output.txt", std::wofstream::app) << text;
#endif
		return true;
	});

	richtextbox->resize(window->getClientSize());
	window->onSized([=](const auto&) { richtextbox->resize(window->getClientSize()); });

	app.run();

	return 0;
}
