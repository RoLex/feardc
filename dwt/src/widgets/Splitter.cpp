/*
  DC++ Widget Toolkit

  Copyright (c) 2007-2022, Jacek Sieka

  All rights reserved.

  Redistribution and use in source and binary forms, with or without modification,
  are permitted provided that the following conditions are met:

      * Redistributions of source code must retain the above copyright notice,
        this list of conditions and the following disclaimer.
      * Redistributions in binary form must reproduce the above copyright notice,
        this list of conditions and the following disclaimer in the documentation
        and/or other materials provided with the distribution.
      * Neither the name of the DWT nor the names of its contributors
        may be used to endorse or promote products derived from this software
        without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
  INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <dwt/widgets/Splitter.h>

#include <vsstyle.h>

#include <dwt/Texts.h>
#include <dwt/WidgetCreator.h>
#include <dwt/resources/Brush.h>
#include <dwt/resources/Pen.h>
#include <dwt/widgets/SplitterContainer.h>
#include <dwt/widgets/ToolTip.h>

namespace dwt {

void Splitter::create(const Seed& cs) {
	pos = cs.pos;
	horizontal = cs.horizontal;
	BaseType::create(cs);

	theme.load(VSCLASS_WINDOW, this);
	onPainting([this](PaintCanvas& canvas) { handlePainting(canvas); });

	onLeftMouseDown([this](const MouseEvent& mouseEvent) { return handleLButtonDown(mouseEvent); });
	onMouseMove([this](const MouseEvent& mouseEvent) { return handleMouseMove(mouseEvent); });
	onLeftMouseUp([this](const MouseEvent&) { return handleLButtonUp(); });

	auto tip = WidgetCreator<ToolTip>::create(this, ToolTip::Seed());
	tip->setText(Texts::get(Texts::resize));
	onDestroy([tip] { tip->close(); });
}

SplitterContainerPtr Splitter::getParent() const {
	return static_cast<SplitterContainerPtr>(BaseType::getParent());
}

void Splitter::handlePainting(PaintCanvas& canvas) {
	if(hovering) {
		if(theme) {
			Rectangle rect { getClientSize() };
			// don't draw edges.
			(horizontal ? rect.pos.x : rect.pos.y) -= 2;
			(horizontal ? rect.size.x : rect.size.y) += 4;

			theme.drawBackground(canvas, WP_CAPTION, CS_ACTIVE, rect, true, canvas.getPaintRect());

		} else {
			canvas.fill(canvas.getPaintRect(), Brush(Brush::ActiveCaption));
		}

	} else {
		auto size = getClientSize();

		Pen pen { Color::predefined(COLOR_3DSHADOW) };
		auto select(canvas.select(pen));

		if(horizontal) {
			auto mid = size.y / 2;
			canvas.line(0, mid - 1, size.x, mid - 1);
			canvas.line(0, mid, size.x, mid);
		} else {
			auto mid = size.x / 2;
			canvas.line(mid - 1, 0, mid - 1, size.y);
			canvas.line(mid, 0, mid, size.y);
		}
	}
}

bool Splitter::handleLButtonDown(const MouseEvent& mouseEvent) {
	::SetCapture(handle());
	moving = mouseEvent.pos.getPoint();
	return true;
}

bool Splitter::handleLButtonUp() {
	::ReleaseCapture();
	moving.reset();
	return true;
}

bool Splitter::handleMouseMove(const MouseEvent& mouseEvent) {
	if(!hovering) {
		hovering = true;
		redraw();
		::SetCursor(::LoadCursor(0, horizontal ? IDC_SIZENS : IDC_SIZEWE));
		onMouseLeave([this] {
			hovering = false;
			redraw();
			::SetCursor(NULL);
		});
	}

	if(moving && mouseEvent.ButtonPressed == MouseEvent::LEFT && mouseEvent.pos.getPoint() != moving) {
		moving = mouseEvent.pos.getPoint();
		ClientCoordinate cc(mouseEvent.pos, getParent());
		double mpos = horizontal ? cc.y() : cc.x();
		double size = horizontal ? getParent()->getClientSize().y : getParent()->getClientSize().x;
		pos = mpos / size;
		getParent()->checkSplitterPos(this);
		getParent()->onMove();
	}

	return true;
}

};
