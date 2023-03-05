/*
 * Copyright (C) 2001-2022 Jacek Sieka, arnetheduck on gmail point com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef DCPLUSPLUS_WIN32_ASPECT_CHAT_H
#define DCPLUSPLUS_WIN32_ASPECT_CHAT_H

#include <dcpp/ChatMessage.h>
#include <dcpp/File.h>
#include <dcpp/SimpleXML.h>
#include <dcpp/Tagger.h>
#include <dcpp/PluginManager.h>

#include <dwt/WidgetCreator.h>

#include "HoldRedraw.h"
#include "HtmlToRtf.h"
#include "RichTextBox.h"
#include "WinUtil.h"

template<typename T>
class AspectChat {
	typedef AspectChat<T> ThisType;

	const T& t() const { return *static_cast<const T*>(this); }
	T& t() { return *static_cast<T*>(this); }

protected:
	AspectChat() :
	chat(0),
	message(0),
	messageLines(1),
	curCommandPosition(0)
	{
	}

	void createChat(dwt::Widget *parent) {
		{
			RichTextBox::Seed cs = WinUtil::Seeds::richTextBox;
			cs.style |= ES_READONLY;
			chat = dwt::WidgetCreator<RichTextBox>::create(parent, cs);
			chat->setTextLimit(1024*64*2);
		}

		{
			TextBox::Seed cs = WinUtil::Seeds::textBox;
			cs.style |= WS_VSCROLL | ES_AUTOVSCROLL | ES_MULTILINE | ES_NOHIDESEL;
			message = t().addChild(cs);
			message->onUpdated([this] { this->handleMessageUpdated(); });
		}

		t().addAccel(FALT, 'C', [this] { this->chat->setFocus(); });
		t().addAccel(FALT, 'M', [this] { this->message->setFocus(); });
		t().addAccel(FALT, 'S', [this] { this->sendMessage(); });
		t().addAccel(0, VK_ESCAPE, [this] { this->handleEscape(); });
		t().addAccel(FCONTROL, 'F', [this] { this->chat->findTextNew(); });
		t().addAccel(0, VK_F3, [this] { this->chat->findTextNext(); });
	}

	virtual ~AspectChat() { }

	/// add a chat message with some formatting and call addedChat.
	void addChat(const tstring& message) {
		string tmp;

		Tagger tags(Text::fromT(message));
		ChatMessage::format(tags, tmp);

		PluginManager::getInstance()->onChatTags(tags);

		string htmlMessage =
			"<span id=\"message\" style=\"white-space: pre-wrap;\">"
			"<span id=\"timestamp\">" + SimpleXML::escape("[" + Util::getShortTimeString() + "]", tmp, false) + "</span> "
			"<span id=\"text\">" + tags.merge(tmp) + "</span>"
			"</span>";

		PluginManager::getInstance()->onChatDisplay(htmlMessage);

		addChatHTML(htmlMessage);
		t().addedChat(message);
	}

	/// add a ChatMessage and call addedChat.
	void addChat(const ChatMessage& message) {
		addChatHTML(message.htmlMessage);
		t().addedChat(message);
	}

	/// add a plain text string directly, with no formatting.
	void addChatPlain(const tstring& message) {
		addChatRTF(dwt::RichTextBox::rtfEscape(message));
	}

	/// add an RTF-formatted message.
	void addChatRTF(tstring message) {
		/// @todo factor out to dwt
		if(chat->length() > 0)
			message = _T("\\line\n") + message;
		chat->addTextSteady(_T("{\\urtf1\n") + message + _T("}\n"));
	}

	/// add an HTML-formatted message.
	void addChatHTML(const string& message) {
		addChatRTF(HtmlToRtf::convert(message, chat));
	}

	void readLog(const string& logPath, const unsigned setting) {
		if(setting == 0)
			return;

		StringList lines;

		try {
			const int MAX_SIZE = 32 * 1024;

			File f(logPath.empty() ? t().getLogPath() : logPath, File::READ, File::OPEN);
			if(f.getSize() > MAX_SIZE) {
				f.setEndPos(-MAX_SIZE + 1);
			}

			lines = StringTokenizer<string>(f.read(MAX_SIZE), "\r\n").getTokens();
		} catch(const FileException&) { }

		if(lines.empty())
			return;

		// the last line in the log file is an empty line; remove it
		lines.pop_back();

		string html;
		string tmp;

		const size_t linesCount = lines.size();
		for(size_t i = (linesCount > setting) ? (linesCount - setting) : 0; i < linesCount; ++i) {
			html += SimpleXML::escape(lines[i], tmp, false) + "<br/>";
		}

		if(!html.empty()) {
			addChatHTML("<span style=\"white-space: pre-wrap; color: #" + Util::cssColor(SETTING(LOG_COLOR)) + ";\">" + html + "</span>");
		}
	}

	bool checkCommand(const tstring& cmd, const tstring& param, tstring& status) {
		if(Util::stricmp(cmd.c_str(), _T("clear")) == 0) {
			unsigned linesToKeep = 0;
			if(!param.empty())
				linesToKeep = Util::toInt(Text::fromT(param));
			if(linesToKeep) {
				unsigned lineCount = chat->getLineCount();
				if(linesToKeep < lineCount) {
					dwt::RichTextBox::HoldScroll hold { chat };
					chat->setSelection(0, chat->lineIndex(lineCount - linesToKeep));
					chat->replaceSelection(_T(""));
				}
			} else {
				chat->setSelection();
				chat->replaceSelection(_T(""));
			}

		} else if(Util::stricmp(cmd.c_str(), _T("f")) == 0) {
			chat->findText(param.empty() ? chat->findTextPopup() : param);

		} else {
			return false;
		}
		return true;
	}

	bool handleMessageKeyDown(int c) {
		switch(c) {
		case VK_RETURN: {
			if(t().isShiftPressed() || t().isControlPressed() || t().isAltPressed()) {
				return false;
			}
			return sendMessage();
		}

		case VK_UP:
			if ( historyActive() ) {
				//scroll up in chat command history
				//currently beyond the last command?
				if (curCommandPosition > 0) {
					//check whether current command needs to be saved
					if (curCommandPosition == prevCommands.size()) {
						currentCommand = message->getText();
					}

					//replace current chat buffer with current command
					message->setText(prevCommands[--curCommandPosition]);
				}
				// move cursor to end of line
				message->setSelection(message->length(), message->length());
				return true;
			}
			break;
		case VK_DOWN:
			if ( historyActive() ) {
				//scroll down in chat command history

				//currently beyond the last command?
				if (curCommandPosition + 1 < prevCommands.size()) {
					//replace current chat buffer with current command
					message->setText(prevCommands[++curCommandPosition]);
				} else if (curCommandPosition + 1 == prevCommands.size()) {
					//revert to last saved, unfinished command

					message->setText(currentCommand);
					++curCommandPosition;
				}
				// move cursor to end of line
				message->setSelection(message->length(), message->length());
				return true;
			}
			break;
		case VK_PRIOR: // page up
			{
				chat->sendMessage(WM_VSCROLL, SB_PAGEUP);
				return true;
			} break;
		case VK_NEXT: // page down
			{
				chat->sendMessage(WM_VSCROLL, SB_PAGEDOWN);
				return true;
			} break;
		case VK_HOME:
			if (!prevCommands.empty() && historyActive() ) {
				curCommandPosition = 0;
				currentCommand = message->getText();

				message->setText(prevCommands[curCommandPosition]);
				return true;
			}
			break;
		case VK_END:
			if (historyActive()) {
				curCommandPosition = prevCommands.size();

				message->setText(currentCommand);
				return true;
			}
			break;
		}
		return false;
	}

	bool handleMessageChar(int c) {
		switch(c) {
		case VK_RETURN: {
			if(!(t().isShiftPressed() || t().isControlPressed() || t().isAltPressed())) {
				return true;
			}
		} break;
		}
		return false;
	}

	void handleEscape() {
		chat->sendMessage(WM_KEYDOWN, VK_ESCAPE);
		message->setFocus();
	}

	RichTextBox* chat;
	dwt::TextBoxPtr message;

	unsigned messageLines;

private:
	TStringList prevCommands;
	tstring currentCommand;
	TStringList::size_type curCommandPosition; //can't use an iterator because StringList is a vector, and vector iterators become invalid after resizing

	void handleMessageUpdated() {
		unsigned lineCount = message->getLineCount();

		// make sure we don't resize to 0 lines...
		const unsigned min_setting = max(SETTING(MIN_MESSAGE_LINES), 1);
		const unsigned max_setting = max(SETTING(MAX_MESSAGE_LINES), 1);

		if(lineCount < min_setting)
			lineCount = min_setting;
		else if(lineCount > max_setting)
			lineCount = max_setting;

		if(lineCount != messageLines) {
			messageLines = lineCount;
			t().layout();
		}
	}

	bool historyActive() const {
		return t().isAltPressed() || (SETTING(USE_CTRL_FOR_LINE_HISTORY) && t().isControlPressed());
	}

	bool sendMessage() {
		tstring s = message->getText();
		if(s.empty()) {
			::MessageBeep(MB_ICONEXCLAMATION);
			return false;
		}

		// save command in history, reset current buffer pointer to the newest command
		curCommandPosition = prevCommands.size();		//this places it one position beyond a legal subscript
		if (curCommandPosition == 0 || (curCommandPosition > 0 && prevCommands[curCommandPosition - 1] != s)) {
			++curCommandPosition;
			prevCommands.push_back(s);
		}
		currentCommand = _T("");

		t().enterImpl(s);
		return true;
	}
};

#endif // !defined(DCPLUSPLUS_WIN32_ASPECT_CHAT_H)
