/*
 * Brisk
 *
 * Cross-platform application framework
 * --------------------------------------------------------------
 *
 * Copyright (C) 2024 Brisk Developers
 *
 * This file is part of the Brisk library.
 *
 * Brisk is dual-licensed under the GNU General Public License, version 2 (GPL-2.0+),
 * and a commercial license. You may use, modify, and distribute this software under
 * the terms of the GPL-2.0+ license if you comply with its conditions.
 *
 * You should have received a copy of the GNU General Public License along with this program.
 * If not, see <http://www.gnu.org/licenses/>.
 *
 * If you do not wish to be bound by the GPL-2.0+ license, you must purchase a commercial
 * license. For commercial licensing options, please visit: https://brisklib.com
 */
#pragma once

#include <brisk/gui/Component.hpp>

namespace Brisk {

/**
 * @brief A component representing a dialog, with buttons to accept or reject.
 *
 * A `DialogComponent` is a special type of component that includes standard dialog behavior
 * such as accepting, rejecting, or closing the dialog.
 */
class DialogComponent : public Component {
public:
    ~DialogComponent();

    /**
     * @brief Accepts the dialog.
     *
     * This is equivalent to the user pressing an "OK" button or accepting the dialog's content.
     */
    void accept();

    /**
     * @brief Rejects the dialog.
     *
     * This is equivalent to the user pressing a "Cancel" button or rejecting the dialog's content.
     */
    void reject();

    /**
     * @brief Closes the dialog with a specific result.
     *
     * @param result The result of the dialog, true for accepted and false for rejected.
     */
    void close(bool result);

    /**
     * @brief Called when the dialog is accepted.
     *
     * Derived classes can override this to implement custom behavior on acceptance.
     */
    virtual void accepted();

    /**
     * @brief Called when the dialog is rejected.
     *
     * Derived classes can override this to implement custom behavior on rejection.
     */
    virtual void rejected();

    bool result = false; ///< The result of the dialog, true if accepted, false if rejected.

    DeferredCallbacks<> onAccepted; ///< Callback triggered when the dialog is accepted.
    DeferredCallbacks<> onRejected; ///< Callback triggered when the dialog is rejected.

protected:
    void unhandledEvent(Event& event) override;

    void configureWindow(RC<GUIWindow> window) override;

    /**
     * @brief Creates buttons for the dialog.
     *
     * This method generates OK and Cancel buttons for the dialog.
     *
     * @param buttons Specifies the types of buttons to create.
     * @param okBtn The label for the OK button. Default is "OK".
     * @param cancelBtn The label for the Cancel button. Default is "Cancel".
     * @return A reference-counted pointer to the created widget containing the buttons.
     */
    RC<Widget> dialogButtons(DialogButtons buttons, std::string okBtn = "OK||Button"_tr,
                             std::string cancelBtn = "Cancel||Button"_tr);
    DialogComponent();
};

/**
 * @brief A dialog component for text input.
 *
 * This dialog presents a prompt and allows the user to input text.
 */
class TextInputDialog final : public DialogComponent {
public:
    /**
     * @brief Constructor for TextInputDialog.
     *
     * @param prompt The prompt text displayed to the user.
     * @param defaultValue The default value of the input field. Defaults to an empty string.
     */
    TextInputDialog(std::string prompt, std::string defaultValue = {});

    std::string prompt; ///< The prompt message displayed to the user.
    std::string value;  ///< The input value entered by the user.

protected:
    RC<Widget> build() override;
};

/**
 * @brief A dialog component for displaying messages with an icon.
 *
 * This dialog is used to show a message along with an optional icon.
 */
class MessageDialog final : public DialogComponent {
public:
    /**
     * @brief Constructor for MessageDialog.
     *
     * @param text The message text to display.
     * @param icon The icon to display alongside the message.
     */
    MessageDialog(std::string text, std::string icon);

    std::string text; ///< The message text.
    std::string icon; ///< The icon displayed alongside the message.

protected:
    RC<Widget> build() override;
};

/**
 * @brief A dialog component for confirming an action.
 *
 * This dialog is used to ask the user to confirm or cancel an action, displaying a message and an icon.
 */
class ConfirmDialog final : public DialogComponent {
public:
    /**
     * @brief Constructor for ConfirmDialog.
     *
     * @param text The confirmation message to display.
     * @param icon The icon to display alongside the message.
     */
    ConfirmDialog(std::string text, std::string icon);

    std::string text; ///< The confirmation message text.
    std::string icon; ///< The icon displayed alongside the confirmation message.

protected:
    RC<Widget> build() override;
};
}
