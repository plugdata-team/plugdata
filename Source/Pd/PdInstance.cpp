/*
 // Copyright (c) 2015-2018 Pierre Guillot.
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#include <algorithm>

extern "C"
{
#include <g_undo.h>
#include <m_imp.h>

#include "s_libpd_inter.h"
#include "x_libpd_extra_utils.h"
#include "x_libpd_mod_utils.h"
#include "x_libpd_multi.h"
}

#include "PdInstance.h"
#include "PdPatch.h"

extern "C"
{
  struct pd::Instance::internal
  {
    static void instance_multi_bang(pd::Instance* ptr, const char* recv) { ptr->m_message_queue.try_enqueue({std::string("bang"), std::string(recv)}); }

    static void instance_multi_float(pd::Instance* ptr, const char* recv, float f) { ptr->m_message_queue.try_enqueue({std::string("float"), std::string(recv), std::vector<Atom>(1, {f})}); }

    static void instance_multi_symbol(pd::Instance* ptr, const char* recv, const char* sym) { ptr->m_message_queue.try_enqueue({std::string("symbol"), std::string(recv), std::vector<Atom>(1, std::string(sym))}); }

    static void instance_multi_list(pd::Instance* ptr, const char* recv, int argc, t_atom* argv)
    {
      Message mess{std::string("list"), std::string(recv), std::vector<Atom>(argc)};
      for (int i = 0; i < argc; ++i)
      {
        if (argv[i].a_type == A_FLOAT)
          mess.list[i] = Atom(atom_getfloat(argv + i));
        else if (argv[i].a_type == A_SYMBOL)
          mess.list[i] = Atom(std::string(atom_getsymbol(argv + i)->s_name));
      }
      ptr->m_message_queue.try_enqueue(std::move(mess));
    }

    static void instance_multi_message(pd::Instance* ptr, const char* recv, const char* msg, int argc, t_atom* argv)
    {
      Message mess{msg, std::string(recv), std::vector<Atom>(argc)};
      for (int i = 0; i < argc; ++i)
      {
        if (argv[i].a_type == A_FLOAT)
          mess.list[i] = Atom(atom_getfloat(argv + i));
        else if (argv[i].a_type == A_SYMBOL)
          mess.list[i] = Atom(std::string(atom_getsymbol(argv + i)->s_name));
      }
      ptr->m_message_queue.try_enqueue(std::move(mess));
    }

    static void instance_multi_noteon(pd::Instance* ptr, int channel, int pitch, int velocity) { ptr->m_midi_queue.try_enqueue({midievent::NOTEON, channel, pitch, velocity}); }

    static void instance_multi_controlchange(pd::Instance* ptr, int channel, int controller, int value) { ptr->m_midi_queue.try_enqueue({midievent::CONTROLCHANGE, channel, controller, value}); }

    static void instance_multi_programchange(pd::Instance* ptr, int channel, int value) { ptr->m_midi_queue.try_enqueue({midievent::PROGRAMCHANGE, channel, value, 0}); }

    static void instance_multi_pitchbend(pd::Instance* ptr, int channel, int value) { ptr->m_midi_queue.try_enqueue({midievent::PITCHBEND, channel, value, 0}); }

    static void instance_multi_aftertouch(pd::Instance* ptr, int channel, int value) { ptr->m_midi_queue.try_enqueue({midievent::AFTERTOUCH, channel, value, 0}); }

    static void instance_multi_polyaftertouch(pd::Instance* ptr, int channel, int pitch, int value) { ptr->m_midi_queue.try_enqueue({midievent::POLYAFTERTOUCH, channel, pitch, value}); }

    static void instance_multi_midibyte(pd::Instance* ptr, int port, int byte) { ptr->m_midi_queue.try_enqueue({midievent::MIDIBYTE, port, byte, 0}); }

    static void instance_multi_print(pd::Instance* ptr, char const* s) { ptr->m_print_queue.try_enqueue(std::string(s)); }
  };
}

namespace pd
{

Instance::Instance(std::string const& symbol)
{
  libpd_multi_init();

  canvasLock.lock();
  m_instance = libpd_new_instance();
  canvasLock.unlock();
  libpd_set_instance(static_cast<t_pdinstance*>(m_instance));
  m_midi_receiver = libpd_multi_midi_new(this, reinterpret_cast<t_libpd_multi_noteonhook>(internal::instance_multi_noteon), reinterpret_cast<t_libpd_multi_controlchangehook>(internal::instance_multi_controlchange),
                                         reinterpret_cast<t_libpd_multi_programchangehook>(internal::instance_multi_programchange), reinterpret_cast<t_libpd_multi_pitchbendhook>(internal::instance_multi_pitchbend), reinterpret_cast<t_libpd_multi_aftertouchhook>(internal::instance_multi_aftertouch),
                                         reinterpret_cast<t_libpd_multi_polyaftertouchhook>(internal::instance_multi_polyaftertouch), reinterpret_cast<t_libpd_multi_midibytehook>(internal::instance_multi_midibyte));
  m_print_receiver = libpd_multi_print_new(this, reinterpret_cast<t_libpd_multi_printhook>(internal::instance_multi_print));

  m_message_receiver[0] = libpd_multi_receiver_new(this, symbol.c_str(), reinterpret_cast<t_libpd_multi_banghook>(internal::instance_multi_bang), reinterpret_cast<t_libpd_multi_floathook>(internal::instance_multi_float), reinterpret_cast<t_libpd_multi_symbolhook>(internal::instance_multi_symbol),
                                                   reinterpret_cast<t_libpd_multi_listhook>(internal::instance_multi_list), reinterpret_cast<t_libpd_multi_messagehook>(internal::instance_multi_message));
  m_atoms = malloc(sizeof(t_atom) * 512);

  // Register callback when pd's gui changes
  // Needs to be done on pd's thread

  register_gui_trigger(static_cast<t_pdinstance*>(m_instance), this,
                       [](void* instance, void* target)
                       {
                         auto* pd = static_cast<t_pd*>(target);

                         // redraw scalar
                         if (pd && !strcmp((*pd)->c_name->s_name, "scalar"))
                         {
                           static_cast<Instance*>(instance)->receiveGuiUpdate(2);
                         }
                         else
                         {
                           static_cast<Instance*>(instance)->receiveGuiUpdate(1);
                         }
                       });

  libpd_set_verbose(0);

  setThis();
}

Instance::~Instance()
{
  closePatch();
  for (auto& i : m_message_receiver) pd_free(static_cast<t_pd*>(i));

  pd_free(static_cast<t_pd*>(m_midi_receiver));
  pd_free(static_cast<t_pd*>(m_print_receiver));

  libpd_set_instance(static_cast<t_pdinstance*>(m_instance));
  libpd_free_instance(static_cast<t_pdinstance*>(m_instance));
}

int Instance::getBlockSize() const noexcept { return libpd_blocksize(); }

void Instance::addListener(const char* sym)
{
  m_message_receiver.push_back(libpd_multi_receiver_new(this, sym, reinterpret_cast<t_libpd_multi_banghook>(internal::instance_multi_bang), reinterpret_cast<t_libpd_multi_floathook>(internal::instance_multi_float), reinterpret_cast<t_libpd_multi_symbolhook>(internal::instance_multi_symbol),
                                                        reinterpret_cast<t_libpd_multi_listhook>(internal::instance_multi_list), reinterpret_cast<t_libpd_multi_messagehook>(internal::instance_multi_message)));
}

void Instance::prepareDSP(const int nins, const int nouts, const double samplerate)
{
  libpd_set_instance(static_cast<t_pdinstance*>(m_instance));
  libpd_init_audio(nins, nouts, static_cast<int>(samplerate));
}

void Instance::startDSP()
{
  t_atom av;
  libpd_set_float(&av, 1.f);
  libpd_message("pd", "dsp", 1, &av);
}

void Instance::releaseDSP()
{
  t_atom av;
  libpd_set_instance(static_cast<t_pdinstance*>(m_instance));
  libpd_set_float(&av, 0.f);
  libpd_message("pd", "dsp", 1, &av);
}

void Instance::performDSP(float const* inputs, float* outputs)
{
  libpd_set_instance(static_cast<t_pdinstance*>(m_instance));
  libpd_process_raw(inputs, outputs);
}

void Instance::sendNoteOn(const int channel, const int pitch, const int velocity) const
{
  libpd_set_instance(static_cast<t_pdinstance*>(m_instance));
  libpd_noteon(channel - 1, pitch, velocity);
}

void Instance::sendControlChange(const int channel, const int controller, const int value) const
{
  libpd_set_instance(static_cast<t_pdinstance*>(m_instance));
  libpd_controlchange(channel - 1, controller, value);
}

void Instance::sendProgramChange(const int channel, const int value) const
{
  libpd_set_instance(static_cast<t_pdinstance*>(m_instance));
  libpd_programchange(channel - 1, value);
}

void Instance::sendPitchBend(const int channel, const int value) const
{
  libpd_set_instance(static_cast<t_pdinstance*>(m_instance));
  libpd_pitchbend(channel - 1, value);
}

void Instance::sendAfterTouch(const int channel, const int value) const
{
  libpd_set_instance(static_cast<t_pdinstance*>(m_instance));
  libpd_aftertouch(channel - 1, value);
}

void Instance::sendPolyAfterTouch(const int channel, const int pitch, const int value) const
{
  libpd_set_instance(static_cast<t_pdinstance*>(m_instance));
  libpd_polyaftertouch(channel - 1, pitch, value);
}

void Instance::sendSysEx(const int port, const int byte) const
{
  libpd_set_instance(static_cast<t_pdinstance*>(m_instance));
  libpd_sysex(port, byte);
}

void Instance::sendSysRealTime(const int port, const int byte) const
{
  libpd_set_instance(static_cast<t_pdinstance*>(m_instance));
  libpd_sysrealtime(port, byte);
}

void Instance::sendMidiByte(const int port, const int byte) const
{
  libpd_set_instance(static_cast<t_pdinstance*>(m_instance));
  libpd_midibyte(port, byte);
}

void Instance::sendBang(const char* receiver) const
{
  if (!m_instance) return;

  libpd_set_instance(static_cast<t_pdinstance*>(m_instance));
  libpd_bang(receiver);
}

void Instance::sendFloat(const char* receiver, float const value) const
{
  if (!m_instance) return;

  libpd_set_instance(static_cast<t_pdinstance*>(m_instance));
  libpd_float(receiver, value);
}

void Instance::sendSymbol(const char* receiver, const char* symbol) const
{
  if (!m_instance) return;

  libpd_set_instance(static_cast<t_pdinstance*>(m_instance));
  libpd_symbol(receiver, symbol);
}

void Instance::sendList(const char* receiver, const std::vector<Atom>& list) const
{
  if (!static_cast<t_pdinstance*>(m_instance)) return;

  auto* argv = static_cast<t_atom*>(m_atoms);
  libpd_set_instance(static_cast<t_pdinstance*>(m_instance));
  for (size_t i = 0; i < list.size(); ++i)
  {
    if (list[i].isFloat())
      libpd_set_float(argv + i, list[i].getFloat());
    else
      libpd_set_symbol(argv + i, list[i].getSymbol().c_str());
  }
  libpd_list(receiver, static_cast<int>(list.size()), argv);
}

void Instance::sendMessage(const char* receiver, const char* msg, const std::vector<Atom>& list) const
{
  if (!static_cast<t_pdinstance*>(m_instance)) return;

  auto* argv = static_cast<t_atom*>(m_atoms);
  libpd_set_instance(static_cast<t_pdinstance*>(m_instance));
  for (size_t i = 0; i < list.size(); ++i)
  {
    if (list[i].isFloat())
      libpd_set_float(argv + i, list[i].getFloat());
    else
      libpd_set_symbol(argv + i, list[i].getSymbol().c_str());
  }
  libpd_message(receiver, msg, static_cast<int>(list.size()), argv);
}

void Instance::processMessages()
{
  Message mess;
  while (m_message_queue.try_dequeue(mess))
  {
    if (mess.selector == "bang")
      receiveBang(mess.destination);
    else if (mess.selector == "float")
      receiveFloat(mess.destination, mess.list[0].getFloat());
    else if (mess.selector == "symbol")
      receiveSymbol(mess.destination, mess.list[0].getSymbol());
    else if (mess.selector == "list")
      receiveList(mess.destination, mess.list);
    else
      receiveMessage(mess.destination, mess.selector, mess.list);
  }
}

void Instance::processMidi()
{
  midievent event;
  while (m_midi_queue.try_dequeue(event))
  {
    if (event.type == midievent::NOTEON)
      receiveNoteOn(event.midi1 + 1, event.midi2, event.midi3);
    else if (event.type == midievent::CONTROLCHANGE)
      receiveControlChange(event.midi1 + 1, event.midi2, event.midi3);
    else if (event.type == midievent::PROGRAMCHANGE)
      receiveProgramChange(event.midi1 + 1, event.midi2);
    else if (event.type == midievent::PITCHBEND)
      receivePitchBend(event.midi1 + 1, event.midi2);
    else if (event.type == midievent::AFTERTOUCH)
      receiveAftertouch(event.midi1 + 1, event.midi2);
    else if (event.type == midievent::POLYAFTERTOUCH)
      receivePolyAftertouch(event.midi1 + 1, event.midi2, event.midi3);
    else if (event.type == midievent::MIDIBYTE)
      receiveMidiByte(event.midi1, event.midi2);
  }
}

void Instance::processPrints()
{
  std::string temp;
  std::string print;
  while (m_print_queue.try_dequeue(print))
  {
    if (!print.empty() && print.back() == '\n')
    {
      while (!print.empty() && (print.back() == '\n' || print.back() == ' '))
      {
        print.pop_back();
      }
      temp += print;

      MessageManager::callAsync([this, temp]() mutable { receivePrint(temp); });

      temp.clear();
    }
    else
    {
      temp += print;
    }
  }
}

void Instance::enqueueFunction(const std::function<void(void)>& fn)
{
  // sys_lock();
  // fn();
  // sys_unlock();

  // This should be the way to do it, but it currently causes some issues
  // By calling fn directly we fix these issues at the cost of possible thread unsafety
  m_function_queue.enqueue(fn);
  messageEnqueued();
}

void Instance::enqueueMessages(const std::string& dest, const std::string& msg, std::vector<Atom>&& list)
{
  m_send_queue.try_enqueue(dmessage{nullptr, dest, msg, std::move(list)});
  messageEnqueued();
}

void Instance::enqueueDirectMessages(void* object, std::vector<Atom> const& list)
{
  m_send_queue.try_enqueue(dmessage{object, std::string(), "list", list});
  messageEnqueued();
}

void Instance::enqueueDirectMessages(void* object, const std::string& msg)
{
  m_send_queue.try_enqueue(dmessage{object, std::string(), "symbol", std::vector<Atom>(1, msg)});
  messageEnqueued();
}

void Instance::enqueueDirectMessages(void* object, const float msg)
{
  m_send_queue.try_enqueue(dmessage{object, std::string(), "float", std::vector<Atom>(1, msg)});
  messageEnqueued();
}

void Instance::waitForStateUpdate()
{
  // Need to wait twice to ensure that pd has processed all changes
  if (audioStarted)
  {
    // Append signal to resume thread at the end of the queue
    // This will make sure that any actions we performed are definitely finished now
    enqueueFunction([this]() { updateWait.signal(); });

    updateWait.wait();
  }
  // Should ensure that patches are loaded correctly when audio hasn't started yet
  else
  {
    std::function<void(void)> callback;
    while (m_function_queue.try_dequeue(callback))
    {
      callback();
    }
  }
}

void Instance::sendMessagesFromQueue()
{
  libpd_set_instance(static_cast<t_pdinstance*>(m_instance));

  std::function<void(void)> callback;
  while (m_function_queue.try_dequeue(callback))
  {
    callback();
    audioStarted = true;
  }

  dmessage mess;
  while (m_send_queue.try_dequeue(mess))
  {
    if (mess.object && !mess.list.empty())
    {
      if (mess.selector == "list")
      {
        auto* argv = static_cast<t_atom*>(m_atoms);
        for (size_t i = 0; i < mess.list.size(); ++i)
        {
          if (mess.list[i].isFloat())
            SETFLOAT(argv + i, mess.list[i].getFloat());
          else if (mess.list[i].isSymbol())
          {
            sys_lock();
            SETSYMBOL(argv + i, gensym(mess.list[i].getSymbol().data()));
            sys_unlock();
          }
          else
            SETFLOAT(argv + i, 0.0);
        }
        sys_lock();
        pd_list(static_cast<t_pd*>(mess.object), gensym("list"), static_cast<int>(mess.list.size()), argv);
        sys_unlock();
      }
      else if (mess.selector == "float" && mess.list[0].isFloat())
      {
        sys_lock();
        pd_float(static_cast<t_pd*>(mess.object), mess.list[0].getFloat());
        sys_unlock();
      }
      else if (mess.selector == "symbol")
      {
        sys_lock();
        pd_symbol(static_cast<t_pd*>(mess.object), gensym(mess.list[0].getSymbol().c_str()));
        sys_unlock();
      }
    }
    else
    {
      sendMessage(mess.destination.c_str(), mess.selector.c_str(), mess.list);
    }
  }

  // canUndo = libpd_can_undo(Patch::getCurrent());
  // canRedo = libpd_can_redo(Patch::getCurrent());
}

Patch Instance::openPatch(const File& toOpen)
{
  String dirname = toOpen.getParentDirectory().getFullPathName();
  auto* dir = dirname.toRawUTF8();

  String filename = toOpen.getFileName();
  auto* file = filename.toRawUTF8();

  closePatch();
  libpd_set_instance(static_cast<t_pdinstance*>(m_instance));

  canvasLock.lock();

  m_patch = libpd_create_canvas(file, dir);

  // canvas_setcurrent(static_cast<t_canvas*>(patch));
  canvasLock.unlock();
  setThis();

  currentFile = toOpen;

  return getPatch();
}

void Instance::savePatch(const File& location)
{
  String fullPathname = location.getParentDirectory().getFullPathName();
  String filename = location.getFileName();

  auto* dir = gensym(fullPathname.toRawUTF8());
  auto* file = gensym(filename.toRawUTF8());
  libpd_savetofile(getPatch().getPointer(), file, dir);

  canvas_dirty(getPatch().getPointer(), 0);
  currentFile = location;
}

void Instance::savePatch()
{
  String fullPathname = currentFile.getParentDirectory().getFullPathName();
  String filename = currentFile.getFileName();

  auto* dir = gensym(fullPathname.toRawUTF8());
  auto* file = gensym(filename.toRawUTF8());

  libpd_savetofile(getPatch().getPointer(), file, dir);

  canvas_dirty(getPatch().getPointer(), 0);
}

bool Instance::isDirty()
{
  if (!m_patch) return false;

  return getPatch().getPointer()->gl_dirty;
}

void Instance::closePatch()
{
  if (m_patch)
  {
    libpd_set_instance(static_cast<t_pdinstance*>(m_instance));
    libpd_closefile(m_patch);
    m_patch = nullptr;
  }
}

Patch Instance::getPatch() { return Patch(m_patch, this); }

Array Instance::getArray(std::string const& name) { return {name, m_instance}; }

void Instance::setThis() { libpd_set_instance(static_cast<t_pdinstance*>(m_instance)); }

}  // namespace pd
