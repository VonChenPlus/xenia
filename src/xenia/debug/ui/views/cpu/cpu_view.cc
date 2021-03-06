/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2015 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#include "xenia/debug/ui/views/cpu/cpu_view.h"

#include "el/animation_manager.h"
#include "xenia/base/string_buffer.h"

namespace xe {
namespace debug {
namespace ui {
namespace views {
namespace cpu {

CpuView::CpuView()
    : View("CPU"),
      gr_registers_control_(RegisterSet::kGeneral),
      fr_registers_control_(RegisterSet::kFloat),
      vr_registers_control_(RegisterSet::kVector),
      host_registers_control_(RegisterSet::kHost) {}

CpuView::~CpuView() = default;

el::Element* CpuView::BuildUI() {
  using namespace el::dsl;  // NOLINT(build/namespaces)
  el::AnimationBlocker animation_blocker;

  auto functions_node =
      LayoutBoxNode()
          .gravity(Gravity::kAll)
          .distribution(LayoutDistribution::kAvailable)
          .axis(Axis::kY)
          .child(LayoutBoxNode()
                     .gravity(Gravity::kTop | Gravity::kLeftRight)
                     .distribution(LayoutDistribution::kAvailable)
                     .axis(Axis::kX)
                     .skin("button_group")
                     .child(DropDownButtonNode().id("module_dropdown")))
          .child(ListBoxNode().id("function_listbox").gravity(Gravity::kAll))
          .child(LayoutBoxNode()
                     .gravity(Gravity::kBottom | Gravity::kLeftRight)
                     .distribution(LayoutDistribution::kAvailable)
                     .axis(Axis::kX)
                     .child(TextBoxNode()
                                .type(EditType::kSearch)
                                .placeholder("Filter")));

  auto register_list_node = ListBoxNode()
                                .gravity(Gravity::kAll)
                                .item("A")
                                .item("A")
                                .item("A")
                                .item("A");
  auto source_registers_node =
      TabContainerNode()
          .gravity(Gravity::kAll)
          .tab(ButtonNode("GR"), LabelNode("<register list control>")
                                     .id("gr_registers_placeholder"))
          .tab(ButtonNode("FR"), LabelNode("<register list control>")
                                     .id("fr_registers_placeholder"))
          .tab(ButtonNode("VR"), LabelNode("<register list control>")
                                     .id("vr_registers_placeholder"))
          .tab(ButtonNode("X64"), LabelNode("<register list control>")
                                      .id("host_registers_placeholder"));

  auto source_tools_node =
      TabContainerNode()
          .gravity(Gravity::kAll)
          .align(Align::kLeft)
          .tab(ButtonNode("Stack"),
               LabelNode("<callstack control>").id("call_stack_placeholder"))
          .tab(ButtonNode("BPs"), LabelNode("BREAKPOINTS"));

  auto source_pane_node =
      LayoutBoxNode()
          .gravity(Gravity::kAll)
          .distribution(LayoutDistribution::kAvailable)
          .axis(Axis::kY)
          .child(
              LayoutBoxNode()
                  .id("source_toolbar")
                  .gravity(Gravity::kTop | Gravity::kLeftRight)
                  .distribution(LayoutDistribution::kGravity)
                  .distribution_position(LayoutDistributionPosition::kLeftTop)
                  .axis(Axis::kX)
                  .child(DropDownButtonNode().id("thread_dropdown")))
          .child(LayoutBoxNode()
                     .id("source_content")
                     .gravity(Gravity::kAll)
                     .distribution(LayoutDistribution::kAvailable)
                     .child(SplitContainerNode()
                                .gravity(Gravity::kAll)
                                .axis(Axis::kX)
                                .fixed_pane(FixedPane::kSecond)
                                .value(250)
                                .pane(SplitContainerNode()
                                          .gravity(Gravity::kAll)
                                          .axis(Axis::kY)
                                          .fixed_pane(FixedPane::kSecond)
                                          .value(240)
                                          .pane(LabelNode("<source control>")
                                                    .id("source_placeholder"))
                                          .pane(source_registers_node))
                                .pane(source_tools_node)));

  auto memory_pane_node = LabelNode("MEMORY");

  auto node = SplitContainerNode()
                  .gravity(Gravity::kAll)
                  .axis(Axis::kY)
                  .min(128)
                  .max(512)
                  .value(128)
                  .pane(functions_node)
                  .pane(SplitContainerNode()
                            .gravity(Gravity::kAll)
                            .axis(Axis::kY)
                            .value(800)
                            .pane(source_pane_node)
                            .pane(memory_pane_node));

  root_element_.set_gravity(Gravity::kAll);
  root_element_.set_layout_distribution(LayoutDistribution::kAvailable);
  root_element_.LoadNodeTree(node);

  el::Label* source_placeholder;
  el::Label* gr_registers_placeholder;
  el::Label* fr_registers_placeholder;
  el::Label* vr_registers_placeholder;
  el::Label* host_registers_placeholder;
  el::Label* call_stack_placeholder;
  root_element_.GetElementsById({
      {TBIDC("source_placeholder"), &source_placeholder},
      {TBIDC("gr_registers_placeholder"), &gr_registers_placeholder},
      {TBIDC("fr_registers_placeholder"), &fr_registers_placeholder},
      {TBIDC("vr_registers_placeholder"), &vr_registers_placeholder},
      {TBIDC("host_registers_placeholder"), &host_registers_placeholder},
      {TBIDC("call_stack_placeholder"), &call_stack_placeholder},
  });
  source_placeholder->parent()->ReplaceChild(source_placeholder,
                                             source_control_.BuildUI());
  source_control_.Setup(client_);
  gr_registers_placeholder->parent()->ReplaceChild(
      gr_registers_placeholder, gr_registers_control_.BuildUI());
  gr_registers_control_.Setup(client_);
  fr_registers_placeholder->parent()->ReplaceChild(
      fr_registers_placeholder, fr_registers_control_.BuildUI());
  fr_registers_control_.Setup(client_);
  vr_registers_placeholder->parent()->ReplaceChild(
      vr_registers_placeholder, vr_registers_control_.BuildUI());
  vr_registers_control_.Setup(client_);
  host_registers_placeholder->parent()->ReplaceChild(
      host_registers_placeholder, host_registers_control_.BuildUI());
  host_registers_control_.Setup(client_);
  call_stack_placeholder->parent()->ReplaceChild(call_stack_placeholder,
                                                 call_stack_control_.BuildUI());
  call_stack_control_.Setup(client_);

  handler_ = std::make_unique<el::EventHandler>(&root_element_);

  handler_->Listen(el::EventType::kChanged, TBIDC("module_dropdown"),
                   [this](const el::Event& ev) {
                     UpdateFunctionList();
                     return true;
                   });
  handler_->Listen(
      el::EventType::kChanged, TBIDC("thread_dropdown"),
      [this](const el::Event& ev) {
        auto thread_dropdown = root_element_.GetElementById<el::DropDownButton>(
            TBIDC("thread_dropdown"));
        auto thread_handle = uint32_t(thread_dropdown->selected_item_id());
        if (thread_handle) {
          current_thread_ = system()->GetThreadByHandle(thread_handle);
        } else {
          current_thread_ = nullptr;
        }
        SwitchCurrentThread(current_thread_);
        return true;
      });

  return &root_element_;
}

void CpuView::Setup(DebugClient* client) {
  client_ = client;

  system()->on_execution_state_changed.AddListener(
      [this]() { UpdateElementState(); });
  system()->on_modules_updated.AddListener([this]() { UpdateModuleList(); });
  system()->on_threads_updated.AddListener([this]() { UpdateThreadList(); });
}

void CpuView::UpdateElementState() {
  root_element_.GetElementsById({
      //
  });
}

void CpuView::UpdateModuleList() {
  el::DropDownButton* module_dropdown;
  root_element_.GetElementsById({
      {TBIDC("module_dropdown"), &module_dropdown},
  });
  auto module_items = module_dropdown->default_source();
  auto modules = system()->modules();
  bool is_first = module_items->size() == 0;
  for (size_t i = module_items->size(); i < modules.size(); ++i) {
    auto module = modules[i];
    auto item = std::make_unique<el::GenericStringItem>(module->name());
    item->id = module->module_handle();
    module_items->push_back(std::move(item));
  }
  if (is_first) {
    module_dropdown->set_value(static_cast<int>(module_items->size() - 1));
  }
}

void CpuView::UpdateFunctionList() {
  el::DropDownButton* module_dropdown;
  el::ListBox* function_listbox;
  root_element_.GetElementsById({
      {TBIDC("module_dropdown"), &module_dropdown},
      {TBIDC("function_listbox"), &function_listbox},
  });
  auto module_handle = module_dropdown->selected_item_id();
  auto module = system()->GetModuleByHandle(module_handle);
  if (!module) {
    function_listbox->default_source()->clear();
    return;
  }
  // TODO(benvanik): fetch list.
}

void CpuView::UpdateThreadList() {
  el::DropDownButton* thread_dropdown;
  root_element_.GetElementsById({
      {TBIDC("thread_dropdown"), &thread_dropdown},
  });
  auto thread_items = thread_dropdown->default_source();
  auto threads = system()->threads();
  bool is_first = thread_items->size() == 0;
  for (size_t i = 0; i < thread_items->size(); ++i) {
    auto thread = threads[i];
    auto item = thread_items->at(i);
    item->str = thread->to_string();
  }
  for (size_t i = thread_items->size(); i < threads.size(); ++i) {
    auto thread = threads[i];
    auto item = std::make_unique<el::GenericStringItem>(thread->to_string());
    item->id = thread->thread_handle();
    thread_items->push_back(std::move(item));
  }
  if (is_first) {
    thread_dropdown->set_value(static_cast<int>(thread_items->size() - 1));
  }
}

void CpuView::SwitchCurrentThread(model::Thread* thread) {
  current_thread_ = thread;
  source_control_.set_thread(thread);
  gr_registers_control_.set_thread(thread);
  fr_registers_control_.set_thread(thread);
  vr_registers_control_.set_thread(thread);
  host_registers_control_.set_thread(thread);
  call_stack_control_.set_thread(thread);
}

}  // namespace cpu
}  // namespace views
}  // namespace ui
}  // namespace debug
}  // namespace xe
