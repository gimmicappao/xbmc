/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PVRContextMenus.h"

#include "ContextMenuItem.h"
#include "ServiceBroker.h"
#include "addons/PVRClient.h"
#include "guilib/GUIWindowManager.h"
#include "utils/URIUtils.h"

#include "pvr/PVRGUIActions.h"
#include "pvr/PVRManager.h"
#include "pvr/channels/PVRChannel.h"
#include "pvr/epg/EpgInfoTag.h"
#include "pvr/recordings/PVRRecording.h"
#include "pvr/recordings/PVRRecordingsPath.h"
#include "pvr/timers/PVRTimers.h"

namespace PVR
{
  namespace CONTEXTMENUITEM
  {
    #define DECL_STATICCONTEXTMENUITEM(clazz) \
    class clazz : public CStaticContextMenuAction \
    { \
    public: \
      explicit clazz(uint32_t label) : CStaticContextMenuAction(label) {} \
      bool IsVisible(const CFileItem &item) const override; \
      bool Execute(const CFileItemPtr &item) const override; \
    };

    #define DECL_CONTEXTMENUITEM(clazz) \
    class clazz : public IContextMenuItem \
    { \
    public: \
      std::string GetLabel(const CFileItem &item) const override; \
      bool IsVisible(const CFileItem &item) const override; \
      bool Execute(const CFileItemPtr &item) const override; \
    };

    DECL_STATICCONTEXTMENUITEM(PlayEpgTag);
    DECL_STATICCONTEXTMENUITEM(PlayRecording);
    DECL_CONTEXTMENUITEM(ShowInformation);
    DECL_STATICCONTEXTMENUITEM(ShowChannelGuide);
    DECL_STATICCONTEXTMENUITEM(FindSimilar);
    DECL_STATICCONTEXTMENUITEM(StartRecording);
    DECL_STATICCONTEXTMENUITEM(StopRecording);
    DECL_STATICCONTEXTMENUITEM(AddTimerRule);
    DECL_CONTEXTMENUITEM(EditTimerRule);
    DECL_STATICCONTEXTMENUITEM(DeleteTimerRule);
    DECL_CONTEXTMENUITEM(EditTimer);
    DECL_CONTEXTMENUITEM(DeleteTimer);
    DECL_STATICCONTEXTMENUITEM(EditRecording);
    DECL_STATICCONTEXTMENUITEM(RenameRecording);
    DECL_CONTEXTMENUITEM(DeleteRecording);
    DECL_STATICCONTEXTMENUITEM(UndeleteRecording);
    DECL_CONTEXTMENUITEM(ToggleTimerState);
    DECL_STATICCONTEXTMENUITEM(RenameTimer);
    DECL_STATICCONTEXTMENUITEM(PVRClientMenuHook);

    CPVRTimerInfoTagPtr GetTimerInfoTagFromItem(const CFileItem &item)
    {
      CPVRTimerInfoTagPtr timer;

      const CPVREpgInfoTagPtr epg(item.GetEPGInfoTag());
      if (epg)
        timer = epg->Timer();

      if (!timer)
        timer = item.GetPVRTimerInfoTag();

      return timer;
    }

    ///////////////////////////////////////////////////////////////////////////////
    // Play epg tag

    bool PlayEpgTag::IsVisible(const CFileItem &item) const
    {
      const CPVREpgInfoTagPtr epg(item.GetEPGInfoTag());
      if (epg)
        return epg->IsPlayable();

      return false;
    }

    bool PlayEpgTag::Execute(const CFileItemPtr &item) const
    {
      return CServiceBroker::GetPVRManager().GUIActions()->PlayEpgTag(item);
    }

    ///////////////////////////////////////////////////////////////////////////////
    // Play recording

    bool PlayRecording::IsVisible(const CFileItem &item) const
    {
      CPVRRecordingPtr recording;

      const CPVREpgInfoTagPtr epg(item.GetEPGInfoTag());
      if (epg)
        recording = epg->Recording();

      if (recording)
        return !recording->IsDeleted();

      return false;
    }

    bool PlayRecording::Execute(const CFileItemPtr &item) const
    {
      return CServiceBroker::GetPVRManager().GUIActions()->PlayRecording(item, true /* bCheckResume */);
    }

    ///////////////////////////////////////////////////////////////////////////////
    // Show information (epg, recording)

    std::string ShowInformation::GetLabel(const CFileItem &item) const
    {
      if (item.GetPVRRecordingInfoTag())
        return g_localizeStrings.Get(19053); /* Recording Information */

      return g_localizeStrings.Get(19047); /* Programme information */
    }

    bool ShowInformation::IsVisible(const CFileItem &item) const
    {
      const CPVRChannelPtr channel(item.GetPVRChannelInfoTag());
      if (channel)
        return channel->GetEPGNow().get() != nullptr;

      if (item.GetEPGInfoTag())
        return true;

      const CPVRTimerInfoTagPtr timer(item.GetPVRTimerInfoTag());
      if (timer && !URIUtils::PathEquals(item.GetPath(), CPVRTimersPath::PATH_ADDTIMER))
        return timer->GetEpgInfoTag().get() != nullptr;

      if (item.GetPVRRecordingInfoTag())
        return true;

      return false;
    }

    bool ShowInformation::Execute(const CFileItemPtr &item) const
    {
      if (item->GetPVRRecordingInfoTag())
        return CServiceBroker::GetPVRManager().GUIActions()->ShowRecordingInfo(item);

      return CServiceBroker::GetPVRManager().GUIActions()->ShowEPGInfo(item);
    }

    ///////////////////////////////////////////////////////////////////////////////
    // Show channel guide

    bool ShowChannelGuide::IsVisible(const CFileItem &item) const
    {
      const CPVRChannelPtr channel(item.GetPVRChannelInfoTag());
      if (channel)
        return channel->GetEPGNow().get() != nullptr;

      return false;
    }

    bool ShowChannelGuide::Execute(const CFileItemPtr &item) const
    {
      return CServiceBroker::GetPVRManager().GUIActions()->ShowChannelEPG(item);
    }

    ///////////////////////////////////////////////////////////////////////////////
    // Find similar

    bool FindSimilar::IsVisible(const CFileItem &item) const
    {
      const CPVRChannelPtr channel(item.GetPVRChannelInfoTag());
      if (channel)
        return channel->GetEPGNow().get() != nullptr;

      if (item.GetEPGInfoTag())
        return true;

      const CPVRRecordingPtr recording(item.GetPVRRecordingInfoTag());
      if (recording)
        return !recording->IsDeleted();

      return false;
    }

    bool FindSimilar::Execute(const CFileItemPtr &item) const
    {
      return CServiceBroker::GetPVRManager().GUIActions()->FindSimilar(item);
    }

    ///////////////////////////////////////////////////////////////////////////////
    // Start recording

    bool StartRecording::IsVisible(const CFileItem &item) const
    {
      const CPVRClientPtr client = CServiceBroker::GetPVRManager().GetClient(item);

      const CPVRChannelPtr channel = item.GetPVRChannelInfoTag();
      if (channel)
        return !channel->IsRecording() && client && client->GetClientCapabilities().SupportsTimers();

      const CPVREpgInfoTagPtr epg = item.GetEPGInfoTag();
      if (epg && !epg->Timer() && epg->Channel() && epg->IsRecordable())
        return client && client->GetClientCapabilities().SupportsTimers();

      return false;
    }

    bool StartRecording::Execute(const CFileItemPtr &item) const
    {
      return CServiceBroker::GetPVRManager().GUIActions()->AddTimer(item, false);
    }

    ///////////////////////////////////////////////////////////////////////////////
    // Stop recording

    bool StopRecording::IsVisible(const CFileItem &item) const
    {
      const CPVRRecordingPtr recording(item.GetPVRRecordingInfoTag());
      if (recording && recording->IsInProgress())
        return true;

      const CPVRChannelPtr channel(item.GetPVRChannelInfoTag());
      if (channel)
        return channel->IsRecording();

      const CPVRTimerInfoTagPtr timer(GetTimerInfoTagFromItem(item));
      if (timer && !URIUtils::PathEquals(item.GetPath(), CPVRTimersPath::PATH_ADDTIMER))
        return timer->IsRecording();

      return false;
    }

    bool StopRecording::Execute(const CFileItemPtr &item) const
    {
      return CServiceBroker::GetPVRManager().GUIActions()->StopRecording(item);
    }

    ///////////////////////////////////////////////////////////////////////////////
    // Edit recording

    bool EditRecording::IsVisible(const CFileItem &item) const
    {
      const CPVRRecordingPtr recording(item.GetPVRRecordingInfoTag());
      if (recording && !recording->IsDeleted() && !recording->IsInProgress())
        return true;

      return false;
    }

    bool EditRecording::Execute(const CFileItemPtr &item) const
    {
      return CServiceBroker::GetPVRManager().GUIActions()->EditRecording(item);
    }

    ///////////////////////////////////////////////////////////////////////////////
    // Rename recording

    bool RenameRecording::IsVisible(const CFileItem &item) const
    {
      const CPVRRecordingPtr recording = item.GetPVRRecordingInfoTag();
      if (recording &&
          !recording->IsDeleted() &&
          !recording->IsInProgress())
      {
        const CPVRClientPtr client = CServiceBroker::GetPVRManager().GetClient(recording->ClientID());
        return client && client->GetClientCapabilities().SupportsRecordingsRename();
      }
      return false;
    }

    bool RenameRecording::Execute(const CFileItemPtr &item) const
    {
      return CServiceBroker::GetPVRManager().GUIActions()->RenameRecording(item);
    }

    ///////////////////////////////////////////////////////////////////////////////
    // Delete recording

    std::string DeleteRecording::GetLabel(const CFileItem &item) const
    {
      const CPVRRecordingPtr recording(item.GetPVRRecordingInfoTag());
      if (recording && recording->IsDeleted())
        return g_localizeStrings.Get(19291); /* Delete permanently */

      return g_localizeStrings.Get(117); /* Delete */
    }

    bool DeleteRecording::IsVisible(const CFileItem &item) const
    {
      const CPVRRecordingPtr recording(item.GetPVRRecordingInfoTag());
      if (recording && !recording->IsInProgress())
        return true;

      // recordings folder?
      if (item.m_bIsFolder)
      {
        const CPVRRecordingsPath path(item.GetPath());
        return path.IsValid();
      }

      return false;
    }

    bool DeleteRecording::Execute(const CFileItemPtr &item) const
    {
      return CServiceBroker::GetPVRManager().GUIActions()->DeleteRecording(item);
    }

    ///////////////////////////////////////////////////////////////////////////////
    // Undelete recording

    bool UndeleteRecording::IsVisible(const CFileItem &item) const
    {
      const CPVRRecordingPtr recording(item.GetPVRRecordingInfoTag());
      if (recording && recording->IsDeleted())
        return true;

      return false;
    }

    bool UndeleteRecording::Execute(const CFileItemPtr &item) const
    {
      return CServiceBroker::GetPVRManager().GUIActions()->UndeleteRecording(item);
    }

    ///////////////////////////////////////////////////////////////////////////////
    // Activate / deactivate timer or timer rule

    std::string ToggleTimerState::GetLabel(const CFileItem &item) const
    {
      const CPVRTimerInfoTagPtr timer(item.GetPVRTimerInfoTag());
      if (timer && timer->m_state != PVR_TIMER_STATE_DISABLED)
        return g_localizeStrings.Get(844); /* Deactivate */

      return g_localizeStrings.Get(843); /* Activate */
    }

    bool ToggleTimerState::IsVisible(const CFileItem &item) const
    {
      const CPVRTimerInfoTagPtr timer(item.GetPVRTimerInfoTag());
      if (!timer || URIUtils::PathEquals(item.GetPath(), CPVRTimersPath::PATH_ADDTIMER))
        return false;

      const CPVRTimerTypePtr timerType(timer->GetTimerType());
      return timerType && timerType->SupportsEnableDisable();
    }

    bool ToggleTimerState::Execute(const CFileItemPtr &item) const
    {
      return CServiceBroker::GetPVRManager().GUIActions()->ToggleTimerState(item);
    }

    ///////////////////////////////////////////////////////////////////////////////
    // Add timer rule

    bool AddTimerRule::IsVisible(const CFileItem &item) const
    {
      const CPVREpgInfoTagPtr epg = item.GetEPGInfoTag();
      if (epg && epg->Channel() && !epg->Timer())
      {
        const CPVRClientPtr client = CServiceBroker::GetPVRManager().GetClient(item);
        return client && client->GetClientCapabilities().SupportsTimers();
      }
      return false;
    }

    bool AddTimerRule::Execute(const CFileItemPtr &item) const
    {
      return CServiceBroker::GetPVRManager().GUIActions()->AddTimerRule(item, true);
    }

    ///////////////////////////////////////////////////////////////////////////////
    // Edit timer rule

    std::string EditTimerRule::GetLabel(const CFileItem &item) const
    {
      const CPVRTimerInfoTagPtr timer(GetTimerInfoTagFromItem(item));
      if (timer && !URIUtils::PathEquals(item.GetPath(), CPVRTimersPath::PATH_ADDTIMER))
      {
        const CPVRTimerInfoTagPtr parentTimer(CServiceBroker::GetPVRManager().Timers()->GetTimerRule(timer));
        if (parentTimer)
        {
          const CPVRTimerTypePtr parentTimerType(parentTimer->GetTimerType());
          if (parentTimerType && !parentTimerType->IsReadOnly())
            return g_localizeStrings.Get(19243); /* Edit timer rule */
        }
      }

      return g_localizeStrings.Get(19304); /* View timer rule */
    }

    bool EditTimerRule::IsVisible(const CFileItem &item) const
    {
      const CPVRTimerInfoTagPtr timer(GetTimerInfoTagFromItem(item));
      if (timer && !URIUtils::PathEquals(item.GetPath(), CPVRTimersPath::PATH_ADDTIMER))
        return timer->GetTimerRuleId() != PVR_TIMER_NO_PARENT;

      return false;
    }

    bool EditTimerRule::Execute(const CFileItemPtr &item) const
    {
      return CServiceBroker::GetPVRManager().GUIActions()->EditTimerRule(item);
    }

    ///////////////////////////////////////////////////////////////////////////////
    // Delete timer rule

    bool DeleteTimerRule::IsVisible(const CFileItem &item) const
    {
      const CPVRTimerInfoTagPtr timer(GetTimerInfoTagFromItem(item));
      if (timer && !URIUtils::PathEquals(item.GetPath(), CPVRTimersPath::PATH_ADDTIMER))
      {
        const CPVRTimerInfoTagPtr parentTimer(CServiceBroker::GetPVRManager().Timers()->GetTimerRule(timer));
        if (parentTimer)
        {
          const CPVRTimerTypePtr parentTimerType(parentTimer->GetTimerType());
          return parentTimerType && parentTimerType->AllowsDelete();
        }
      }

      return false;
    }

    bool DeleteTimerRule::Execute(const CFileItemPtr &item) const
    {
      const CFileItemPtr parentTimer(CServiceBroker::GetPVRManager().Timers()->GetTimerRule(item));
      if (parentTimer)
        return CServiceBroker::GetPVRManager().GUIActions()->DeleteTimerRule(parentTimer);

      return false;
    }

    ///////////////////////////////////////////////////////////////////////////////
    // Edit / View timer

    std::string EditTimer::GetLabel(const CFileItem &item) const
    {
      const CPVRTimerInfoTagPtr timer(GetTimerInfoTagFromItem(item));
      if (timer)
      {
        const CPVRTimerTypePtr timerType(timer->GetTimerType());
        if (timerType)
        {
          const CPVREpgInfoTagPtr epg(item.GetEPGInfoTag());
          if (!timerType->IsReadOnly() && timer->GetTimerRuleId() == PVR_TIMER_NO_PARENT)
          {
            if (epg)
              return g_localizeStrings.Get(19242); /* Edit timer */
            else
              return g_localizeStrings.Get(21450); /* Edit */
          }
          else
          {
            if (!epg)
              return g_localizeStrings.Get(21483); /* View */
          }
        }
      }

      return g_localizeStrings.Get(19241); /* View timer information */
    }

    bool EditTimer::IsVisible(const CFileItem &item) const
    {
      const CPVRTimerInfoTagPtr timer(GetTimerInfoTagFromItem(item));
      return timer && (!item.GetEPGInfoTag() || !URIUtils::PathEquals(item.GetPath(), CPVRTimersPath::PATH_ADDTIMER));
    }

    bool EditTimer::Execute(const CFileItemPtr &item) const
    {
      return CServiceBroker::GetPVRManager().GUIActions()->EditTimer(item);
    }

    ///////////////////////////////////////////////////////////////////////////////
    // Rename timer

    bool RenameTimer::IsVisible(const CFileItem &item) const
    {
      const CPVRTimerInfoTagPtr timer(item.GetPVRTimerInfoTag());
      if (!timer || URIUtils::PathEquals(item.GetPath(), CPVRTimersPath::PATH_ADDTIMER))
        return false;

      // As epg-based timers will get it's title from the epg tag, they should not be renamable.
      if (timer->IsManual())
      {
        const CPVRTimerTypePtr timerType(timer->GetTimerType());
        if (!timerType->IsReadOnly())
          return true;
      }

      return false;
    }

    bool RenameTimer::Execute(const CFileItemPtr &item) const
    {
      return CServiceBroker::GetPVRManager().GUIActions()->RenameTimer(item);
    }

    ///////////////////////////////////////////////////////////////////////////////
    // Delete timer

    std::string DeleteTimer::GetLabel(const CFileItem &item) const
    {
      if (item.GetPVRTimerInfoTag())
        return g_localizeStrings.Get(117); /* Delete */

      return g_localizeStrings.Get(19060); /* Delete timer */
    }

    bool DeleteTimer::IsVisible(const CFileItem &item) const
    {
      const CPVRTimerInfoTagPtr timer(GetTimerInfoTagFromItem(item));
      if (timer && (!item.GetEPGInfoTag() || !URIUtils::PathEquals(item.GetPath(), CPVRTimersPath::PATH_ADDTIMER)) && !timer->IsRecording())
      {
        const CPVRTimerTypePtr timerType(timer->GetTimerType());
        return  timerType && timerType->AllowsDelete();
      }

      return false;
    }

    bool DeleteTimer::Execute(const CFileItemPtr &item) const
    {
      return CServiceBroker::GetPVRManager().GUIActions()->DeleteTimer(item);
    }

    ///////////////////////////////////////////////////////////////////////////////
    // PVR Client menu hook

    bool PVRClientMenuHook::IsVisible(const CFileItem &item) const
    {
      const CPVRClientPtr client = CServiceBroker::GetPVRManager().GetClient(item);
      if (!client)
        return false;

      PVR_MENUHOOK_CAT cat = PVR_MENUHOOK_UNKNOWN;

      if (item.HasPVRChannelInfoTag())
        cat = PVR_MENUHOOK_CHANNEL;
      else if (item.HasEPGInfoTag())
        cat = PVR_MENUHOOK_EPG;
      else if (item.HasPVRTimerInfoTag() &&
               !URIUtils::PathEquals(item.GetPath(), CPVRTimersPath::PATH_ADDTIMER))
        cat = PVR_MENUHOOK_TIMER;
      else if (item.HasPVRRecordingInfoTag())
      {
        const CPVRRecordingPtr recording = item.GetPVRRecordingInfoTag();
        if (recording->IsDeleted())
          cat = PVR_MENUHOOK_DELETED_RECORDING;
        else
          cat = PVR_MENUHOOK_RECORDING;
      }

      if (cat == PVR_MENUHOOK_UNKNOWN)
        return false;

      return client->HasMenuHooks(cat);
    }

    bool PVRClientMenuHook::Execute(const CFileItemPtr &item) const
    {
      return CServiceBroker::GetPVRManager().GUIActions()->ProcessMenuHooks(item);
    }

  } // namespace CONEXTMENUITEM

  CPVRContextMenuManager& CPVRContextMenuManager::GetInstance()
  {
    static CPVRContextMenuManager instance;
    return instance;
  }

  CPVRContextMenuManager::CPVRContextMenuManager()
  {
    m_items =
    {
      std::make_shared<CONTEXTMENUITEM::PlayEpgTag>(19190), /* Play programme */
      std::make_shared<CONTEXTMENUITEM::PlayRecording>(19687), /* Play recording */
      std::make_shared<CONTEXTMENUITEM::ShowInformation>(),
      std::make_shared<CONTEXTMENUITEM::ShowChannelGuide>(19686), /* Channel guide */
      std::make_shared<CONTEXTMENUITEM::FindSimilar>(19003), /* Find similar */
      std::make_shared<CONTEXTMENUITEM::ToggleTimerState>(),
      std::make_shared<CONTEXTMENUITEM::AddTimerRule>(19061), /* Add timer */
      std::make_shared<CONTEXTMENUITEM::EditTimerRule>(),
      std::make_shared<CONTEXTMENUITEM::DeleteTimerRule>(19295), /* Delete timer rule */
      std::make_shared<CONTEXTMENUITEM::EditTimer>(),
      std::make_shared<CONTEXTMENUITEM::RenameTimer>(118), /* Rename */
      std::make_shared<CONTEXTMENUITEM::DeleteTimer>(),
      std::make_shared<CONTEXTMENUITEM::StartRecording>(264), /* Record */
      std::make_shared<CONTEXTMENUITEM::StopRecording>(19059), /* Stop recording */
      std::make_shared<CONTEXTMENUITEM::EditRecording>(21450), /* Edit */
      std::make_shared<CONTEXTMENUITEM::RenameRecording>(118), /* Rename */
      std::make_shared<CONTEXTMENUITEM::DeleteRecording>(),
      std::make_shared<CONTEXTMENUITEM::UndeleteRecording>(19290), /* Undelete */
      std::make_shared<CONTEXTMENUITEM::PVRClientMenuHook>(19195), /* PVR client specific action */
    };
  }

} // namespace PVR
