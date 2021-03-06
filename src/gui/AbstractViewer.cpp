/*
	This file is part of NSEssentials.

	Use of this source code is granted via a BSD-style license, which can be found
	in License.txt in the repository root.

	@author Nico Schertler
*/

#ifdef HAVE_NANOGUI

#include "nsessentials/gui/AbstractViewer.h"

using namespace nse::gui;

AbstractViewer::AbstractViewer(const std::string& title, int width, int height, int nSamples)
	: nanogui::Screen(Eigen::Vector2i(width, height), title, true, false, 8, 8, 24, 8, nSamples),
	_camera(*this), _ctrlDown(false), _shiftDown(false), nSamples(nSamples)
{
	
}

bool AbstractViewer::keyboardEvent(int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_LEFT_CONTROL && action == 0)
		_ctrlDown = false;
	if (key == GLFW_KEY_LEFT_CONTROL && action == 1)
		_ctrlDown = true;

	if (key == GLFW_KEY_LEFT_SHIFT && action == 0)
		_shiftDown = false;
	if (key == GLFW_KEY_LEFT_SHIFT && action == 1)
		_shiftDown = true;

	return true;
}

bool AbstractViewer::scrollEvent(const Eigen::Vector2i &p, const Eigen::Vector2f &rel)
{
	if (Screen::scrollEvent(p, rel))
		return true;

	if (scrollHook(p, rel))
		return true;

	if (!_ctrlDown && !_shiftDown)
	{
		_camera.Zoom(rel.y());
		return true;
	}

	return false;
}

bool AbstractViewer::mouseButtonEvent(const Eigen::Vector2i &p, int button, bool down, int modifiers)
{
	if (Screen::mouseButtonEvent(p, button, down, modifiers) && down)
		return true;

	if (mouseButtonHook(p, button, down, modifiers) && down)
		return true;

	auto now = std::chrono::high_resolution_clock::now();
	if (std::chrono::duration_cast<std::chrono::milliseconds>(now - lastClickTime).count() < 300 && !down && button == lastClickButton && (p - lastClickPosition).cwiseAbs().sum() <= 2)
	{
		if (button == GLFW_MOUSE_BUTTON_LEFT)
		{
			Eigen::Vector3f point;
			auto depth = get3DPosition(p, point);
			if (depth < 1)
			{
				_camera.FocusOnPoint(point);
				glfwSetCursorPos(mGLFWWindow, width() / 2, height() / 2);
			}
		}
	}

	if (!down)
	{
		lastClickTime = now;
		lastClickButton = button;
		lastClickPosition = p;
	}

	return _camera.HandleMouseButton(p, button, down, modifiers);	
}

bool AbstractViewer::mouseMotionEvent(const Eigen::Vector2i &p, const Eigen::Vector2i &rel,
	int button, int modifiers)
{
	if (Screen::mouseMotionEvent(p, rel, button, modifiers))
		return true;

	if (mouseMotionHook(p, rel, button, modifiers))
		return true;

	return _camera.HandleMouseMove(p, rel, button, modifiers);
}

bool AbstractViewer::resizeEvent(const Eigen::Vector2i & s)
{
	Screen::resizeEvent(s);
	_camera.resize(s);
	return true;
}

float AbstractViewer::get3DPosition(const Eigen::Vector2i & screenPos, Eigen::Vector4f & pos)
{
	float depth;
	glReadPixels(screenPos.x(), height() - 1 - screenPos.y(), 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &depth);

	float ndcDepth = 2 * (depth - 0.5f);

	float x = 2 * ((float)screenPos.x() / width() - 0.5f);
	float y = 2 * ((float)-screenPos.y() / height() + 0.5f);

	Eigen::Matrix4f view, proj;
	camera().ComputeCameraMatrices(view, proj);

	Eigen::Matrix4f mvp = proj * view;
	Eigen::Matrix4f invMvp = mvp.inverse();

	pos = invMvp * Eigen::Vector4f(x, y, ndcDepth, 1);
	pos /= pos.w();

	return depth;
}

float AbstractViewer::get3DPosition(const Eigen::Vector2i & screenPos, Eigen::Vector3f & pos)
{
	Eigen::Vector4f pos4;
	float depth = get3DPosition(screenPos, pos4);
	pos.x() = pos4.x();
	pos.y() = pos4.y();
	pos.z() = pos4.z();
	return depth;
}
#endif