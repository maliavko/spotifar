#ifndef LNG_HPP_A5DED539_88BF_4605_9A46_F2D47A6D9F7C
#define LNG_HPP_A5DED539_88BF_4605_9A46_F2D47A6D9F7C
#pragma once

namespace spotifar
{
    enum
    {
        MPluginUserName,

        MOk,
        MCancel,
        MYes,
        MShowLogs,
        MRelaunch,
        MLike,
        MUnlike,

        MSortDialogTitle,

        // config menu
        MConfigDialogMenuTitle,
        MConfigDialogMenuGeneralOpt,
        MConfigDialogMenuBackendOpt,
        MConfigDialogMenuHotkeysOpt,

        // general configuration dialog
        MConfigGeneralBoxTitle,
        MConfigAddToDisksMenu,
        MConfigVerboseLoggingSetting,
        MConfigSpotifyBlockTitle,
        MConfigSpotifyClientID,
        MConfigSpotifyClientSecret,
        MConfigLocalhostServicePort,
        MConfigNotificationsBlockTitle,
        MConfigTrackChangedSetting,
        MConfigImageShapeSetting,

        // playback configuration dialog
        MConfigPlaybackSetting,
        MConfigNormalisationSetting,
        MConfigAutoplaySetting,
        MConfigGaplessSetting,
        MConfigCacheSetting,
        MConfigBitrateSetting,
        MConfigFormatSetting,
        MConfigDitherSetting,
        MConfigVolumeCtrlSetting,

        // hotkeys configuration dialog
        MConfigHotkeysBoxLabel,
        MConfigHotkeysTableTitle,
        MConfigPlayPauseSetting,
        MConfigSkipToNextSetting,
        MConfigSkipToPrevSetting,
        MConfigSeekForwardSetting,
        MConfigSeekBackwardSetting,
        MConfigVolumeUpSetting,
        MConfigVolumeDownSetting,
        MConfigShowToastSetting,

        MPanelCollectionItemLabel,
        MPanelCollectionItemDescr,
        MPanelArtistsItemLabel,
        MPanelArtistsItemDescr,
        MPanelAlbumsItemLabel,
        MPanelAlbumsItemDescr,
        MPanelTracksItemLabel,
        MPanelTracksItemDescr,
        MPanelAlbumItemLabel,
        MPanelAlbumItemDescr,
        MPanelArtistItemLabel,
        MPanelArtistItemDescr,
        MPanelPlaylistsItemLabel,
        MPanelPlaylistsItemDescr,
		MPanelRecentsItemLabel,
		MPanelRecentsItemDescr,
		MPanelPlayingQueueItemLabel,
		MPanelPlayingQueueItemDescr,
		MPanelRecentTracksItemLabel,
		MPanelRecentTracksItemDescr,
		MPanelRecentAlbumsItemLabel,
		MPanelRecentAlbumsItemDescr,
		MPanelRecentArtistsItemLabel,
		MPanelRecentArtistsItemDescr,
		MPanelRecentPlaylistsItemLabel,
		MPanelRecentPlaylistsItemDescr,
		MPanelNewReleasesItemLabel,
		MPanelNewReleasesItemDescr,
		MPanelBrowseItemLabel,
		MPanelBrowseItemDescr,
		MPanelRecentlySavedLabel,
		MPanelRecentlySavedDescr,
		MPanelRecentlyLikedTracksLabel,
		MPanelRecentlyLikedTracksDescr,
		MPanelRecentlySavedAlbumsLabel,
		MPanelRecentlySavedAlbumsDescr,
		MPanelUserTopItemsLabel,
		MPanelUserTopItemsDescr,
		MPanelUserTopArtistsLabel,
		MPanelUserTopArtistsDescr,
		MPanelUserTopTracksLabel,
		MPanelUserTopTracksDescr,
		MPanelArtistTopTracksLabel,
		MPanelArtistTopTracksDescr,

        MPlayerSourceLabel,
        MPlayerSourceCollection,
        MPlayerRepeatNoneBtn,
        MPlayerRepeatOneBtn,
        MPlayerRepeatAllBtn,
        MPlayerShuffleBtn,

        // spotify specifics
        MArtistUnknown,
        MGenreUnknown,
        MCopyrightUnknown,

        // warnings & errors
        MFarMessageErrorTitle,
        MErrorPluginStartupUnexpected,
        MErrorPluginStartupNoClientID,
        MErrorPluginStartupNoClientSecret,
        MErrorLibrespotStartupUnexpected,
        MErrorLibrespotStoppedUnexpectedly,
        MErrorWinToastStartupUnexpected,
        MErrorSyncThreadFailed,
        MErrorPlaybackCmdFailed,
        MErrorCollectionFetchFailed,
        MErrorConfigMigrationError,

        // notifications
        MToastNewReleasesFoundTitle,
        MToastNewReleasesFoundHaveALookBtn,
        MToastSpotifyAttibution,
        MToastTrackChangedLikeBtn,
        MToastTrackChangedUnlikeBtn,
        MToastTrackChangedNextBtn,

        // complimentary dialogs & menus
        MTransferPlaybackTitle,
        MTransferPlaybackMessage,
        MTransferPlaybackInactiveMessage01,
        MTransferPlaybackInactiveMessage02,
        MTransferNoAvailableDevices,
        MWaitingRequest,
        MWaitingItemsProgress,
        MWaitingInitSpotify,
        MWaitingInitSyncWorker,
        MWaitingInitLibrespot,
        MWaitingFiniSpotify,
        MWaitingFiniSyncWorker,
        MWaitingFiniLibrespot,
    };
}

#endif // LNG_HPP_A5DED539_88BF_4605_9A46_F2D47A6D9F7C