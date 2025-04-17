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
        MError,
        MYes,

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

        MPanelRootItemLabel,
        MPanelRootItemDescr,
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
		MPanelFeaturingArtistsItemLabel,
		MPanelFeaturingArtistsItemDescr,
		MPanelFeaturingAlbumsItemLabel,
		MPanelFeaturingAlbumsItemDescr,

        MPlayerSourceLabel,
        MPlayerSourceCollection,
        MPlayerRepeatNoneBtn,
        MPlayerRepeatOneBtn,
        MPlayerRepeatAllBtn,
        MPlayerShuffleBtn,

        // spotify specifics
        MArtistUnknown,

        // warnings & errors
        MFarMessageErrorTitle,

        MErrorPluginStartupUnexpected,
        MErrorPluginStartupNoClientID,
        MErrorPluginStartupNoClientSecret,
        MErrorLibrespotStartupUnexpected,
        MErrorWinToastStartupUnexpected,

        // complimentary dialogs & menus
        MTransferPlaybackTitle,
        MTransferPlaybackMessage,
    };
}

#endif // LNG_HPP_A5DED539_88BF_4605_9A46_F2D47A6D9F7C