
#include "StdAfx.h"
#include "DmapParser.h"

// Code by Matt Stevens
// https://github.com/mattstevens/dmap-parser
//
//
// Copyright (c) 2011-2013 Matt Stevens
// 
// 
// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:
// 
// 
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
// 
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
// LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
// OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
// WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
// 

typedef enum {
	DMAP_UNKNOWN,
	DMAP_UINT,
	DMAP_INT,
	DMAP_STR,
	DMAP_DATE,
	DMAP_VERS,
	DMAP_DICT
} DMAP_FIELD;


typedef struct {
	const char *code;
	DMAP_FIELD type;
	const char *name;
} dmap_type;


static const dmap_type dmap_types[] = {
	{ "abal", DMAP_DICT, "daap.browsealbumlisting" },
	{ "abar", DMAP_DICT, "daap.browseartistlisting" },
	{ "abcp", DMAP_DICT, "daap.browsecomposerlisting" },
	{ "abgn", DMAP_DICT, "daap.browsegenrelisting" },
	{ "abpl", DMAP_UINT, "daap.baseplaylist" },
	{ "abro", DMAP_DICT, "daap.databasebrowse" },
	{ "adbs", DMAP_DICT, "daap.databasesongs" },
	{ "aeAD", DMAP_DICT, "com.apple.itunes.adam-ids-array" },
	{ "aeAI", DMAP_UINT, "com.apple.itunes.itms-artistid" },
	{ "aeCF", DMAP_UINT, "com.apple.itunes.cloud-flavor-id" },
	{ "aeCI", DMAP_UINT, "com.apple.itunes.itms-composerid" },
	{ "aeCK", DMAP_UINT, "com.apple.itunes.cloud-library-kind" },
	{ "aeCM", DMAP_UINT, "com.apple.itunes.cloud-status" },
	{ "aeCR", DMAP_STR,  "com.apple.itunes.content-rating" } ,
	{ "aeCS", DMAP_UINT, "com.apple.itunes.artworkchecksum" },
	{ "aeCU", DMAP_UINT, "com.apple.itunes.cloud-user-id" },
	{ "aeCd", DMAP_UINT, "com.apple.itunes.cloud-id" },
	{ "aeDP", DMAP_UINT, "com.apple.itunes.drm-platform-id" },
	{ "aeDR", DMAP_UINT, "com.apple.itunes.drm-user-id" },
	{ "aeDV", DMAP_UINT, "com.apple.itunes.drm-versions" },
	{ "aeEN", DMAP_STR,  "com.apple.itunes.episode-num-str" },
	{ "aeES", DMAP_UINT, "com.apple.itunes.episode-sort" },
	{ "aeGD", DMAP_UINT, "com.apple.itunes.gapless-enc-dr" } ,
	{ "aeGE", DMAP_UINT, "com.apple.itunes.gapless-enc-del" },
	{ "aeGH", DMAP_UINT, "com.apple.itunes.gapless-heur" },
	{ "aeGI", DMAP_UINT, "com.apple.itunes.itms-genreid" },
	{ "aeGR", DMAP_UINT, "com.apple.itunes.gapless-resy" },
	{ "aeGU", DMAP_UINT, "com.apple.itunes.gapless-dur" },
	{ "aeGs", DMAP_UINT, "com.apple.itunes.can-be-genius-seed" },
	{ "aeHD", DMAP_UINT, "com.apple.itunes.is-hd-video" },
	{ "aeHV", DMAP_UINT, "com.apple.itunes.has-video" },
	{ "aeK1", DMAP_UINT, "com.apple.itunes.drm-key1-id" },
	{ "aeK2", DMAP_UINT, "com.apple.itunes.drm-key2-id" },
	{ "aeMC", DMAP_UINT, "com.apple.itunes.playlist-contains-media-type-count" },
	{ "aeMK", DMAP_UINT, "com.apple.itunes.mediakind" },
	{ "aeMX", DMAP_STR,  "com.apple.itunes.movie-info-xml" },
	{ "aeMk", DMAP_UINT, "com.apple.itunes.extended-media-kind" },
	{ "aeND", DMAP_UINT, "com.apple.itunes.non-drm-user-id" },
	{ "aeNN", DMAP_STR,  "com.apple.itunes.network-name" },
	{ "aeNV", DMAP_UINT, "com.apple.itunes.norm-volume" },
	{ "aePC", DMAP_UINT, "com.apple.itunes.is-podcast" },
	{ "aePI", DMAP_UINT, "com.apple.itunes.itms-playlistid" },
	{ "aePP", DMAP_UINT, "com.apple.itunes.is-podcast-playlist" },
	{ "aePS", DMAP_UINT, "com.apple.itunes.special-playlist" },
	{ "aeRD", DMAP_UINT, "com.apple.itunes.rental-duration" },
	{ "aeRP", DMAP_UINT, "com.apple.itunes.rental-pb-start" },
	{ "aeRS", DMAP_UINT, "com.apple.itunes.rental-start" },
	{ "aeRU", DMAP_UINT, "com.apple.itunes.rental-pb-duration" },
	{ "aeSE", DMAP_UINT, "com.apple.itunes.store-pers-id" },
	{ "aeSF", DMAP_UINT, "com.apple.itunes.itms-storefrontid" },
	{ "aeSG", DMAP_UINT, "com.apple.itunes.saved-genius" },
	{ "aeSI", DMAP_UINT, "com.apple.itunes.itms-songid" },
	{ "aeSN", DMAP_STR,  "com.apple.itunes.series-name" },
	{ "aeSP", DMAP_UINT, "com.apple.itunes.smart-playlist" },
	{ "aeSU", DMAP_UINT, "com.apple.itunes.season-num" },
	{ "aeSV", DMAP_VERS, "com.apple.itunes.music-sharing-version" },
	{ "aeXD", DMAP_STR,  "com.apple.itunes.xid" },
	{ "agac", DMAP_UINT, "daap.groupalbumcount" },
	{ "agma", DMAP_UINT, "daap.groupmatchedqueryalbumcount" },
	{ "agmi", DMAP_UINT, "daap.groupmatchedqueryitemcount" },
	{ "agrp", DMAP_STR,  "daap.songgrouping" },
	{ "aply", DMAP_DICT, "daap.databaseplaylists" },
	{ "aprm", DMAP_UINT, "daap.playlistrepeatmode" },
	{ "apro", DMAP_VERS, "daap.protocolversion" },
	{ "apsm", DMAP_UINT, "daap.playlistshufflemode" },
	{ "apso", DMAP_DICT, "daap.playlistsongs" },
	{ "arif", DMAP_DICT, "daap.resolveinfo" },
	{ "arsv", DMAP_DICT, "daap.resolve" },
	{ "asaa", DMAP_STR,  "daap.songalbumartist" },
	{ "asac", DMAP_UINT, "daap.songartworkcount" },
	{ "asai", DMAP_UINT, "daap.songalbumid" },
	{ "asal", DMAP_STR,  "daap.songalbum" },
	{ "asar", DMAP_STR,  "daap.songartist" },
	{ "asas", DMAP_UINT, "daap.songalbumuserratingstatus" },
	{ "asbk", DMAP_UINT, "daap.bookmarkable" },
	{ "asbo", DMAP_UINT, "daap.songbookmark" },
	{ "asbr", DMAP_UINT, "daap.songbitrate" },
	{ "asbt", DMAP_UINT, "daap.songbeatsperminute" },
	{ "ascd", DMAP_UINT, "daap.songcodectype" },
	{ "ascm", DMAP_STR,  "daap.songcomment" },
	{ "ascn", DMAP_STR,  "daap.songcontentdescription" },
	{ "asco", DMAP_UINT, "daap.songcompilation" },
	{ "ascp", DMAP_STR,  "daap.songcomposer" },
	{ "ascr", DMAP_UINT, "daap.songcontentrating" },
	{ "ascs", DMAP_UINT, "daap.songcodecsubtype" },
	{ "asct", DMAP_STR,  "daap.songcategory" },
	{ "asda", DMAP_DATE, "daap.songdateadded" },
	{ "asdb", DMAP_UINT, "daap.songdisabled" },
	{ "asdc", DMAP_UINT, "daap.songdisccount" },
	{ "asdk", DMAP_UINT, "daap.songdatakind" },
	{ "asdm", DMAP_DATE, "daap.songdatemodified" },
	{ "asdn", DMAP_UINT, "daap.songdiscnumber" },
	{ "asdp", DMAP_DATE, "daap.songdatepurchased" },
	{ "asdr", DMAP_DATE, "daap.songdatereleased" },
	{ "asdt", DMAP_STR,  "daap.songdescription" },
	{ "ased", DMAP_UINT, "daap.songextradata" },
	{ "aseq", DMAP_STR,  "daap.songeqpreset" },
	{ "ases", DMAP_UINT, "daap.songexcludefromshuffle" },
	{ "asfm", DMAP_STR,  "daap.songformat" },
	{ "asgn", DMAP_STR,  "daap.songgenre" },
	{ "asgp", DMAP_UINT, "daap.songgapless" },
	{ "asgr", DMAP_UINT, "daap.supportsgroups" },
	{ "ashp", DMAP_UINT, "daap.songhasbeenplayed" },
	{ "askd", DMAP_DATE, "daap.songlastskipdate" },
	{ "askp", DMAP_UINT, "daap.songuserskipcount" },
	{ "asky", DMAP_STR,  "daap.songkeywords" },
	{ "aslc", DMAP_STR,  "daap.songlongcontentdescription" },
	{ "aslr", DMAP_UINT, "daap.songalbumuserrating" },
	{ "asls", DMAP_UINT, "daap.songlongsize" },
	{ "aspc", DMAP_UINT, "daap.songuserplaycount" },
	{ "aspl", DMAP_DATE, "daap.songdateplayed" },
	{ "aspu", DMAP_STR,  "daap.songpodcasturl" },
	{ "asri", DMAP_UINT, "daap.songartistid" },
	{ "asrs", DMAP_UINT, "daap.songuserratingstatus" },
	{ "asrv", DMAP_INT,  "daap.songrelativevolume" },
	{ "assa", DMAP_STR,  "daap.sortartist" },
	{ "assc", DMAP_STR,  "daap.sortcomposer" },
	{ "assl", DMAP_STR,  "daap.sortalbumartist" },
	{ "assn", DMAP_STR,  "daap.sortname" },
	{ "assp", DMAP_UINT, "daap.songstoptime" },
	{ "assr", DMAP_UINT, "daap.songsamplerate" },
	{ "asss", DMAP_STR,  "daap.sortseriesname" },
	{ "asst", DMAP_UINT, "daap.songstarttime" },
	{ "assu", DMAP_STR,  "daap.sortalbum" },
	{ "assz", DMAP_UINT, "daap.songsize" },
	{ "astc", DMAP_UINT, "daap.songtrackcount" },
	{ "astm", DMAP_UINT, "daap.songtime" },
	{ "astn", DMAP_UINT, "daap.songtracknumber" },
	{ "asul", DMAP_STR,  "daap.songdataurl" },
	{ "asur", DMAP_UINT, "daap.songuserrating" },
	{ "asvc", DMAP_UINT, "daap.songprimaryvideocodec" },
	{ "asyr", DMAP_UINT, "daap.songyear" },
	{ "ated", DMAP_UINT, "daap.supportsextradata" },
	{ "avdb", DMAP_DICT, "daap.serverdatabases" },
	{ "caar", DMAP_UINT, "dacp.availablerepeatstates" },
	{ "caas", DMAP_UINT, "dacp.availableshufflestates" },
	{ "caci", DMAP_DICT, "caci" },
	{ "cafe", DMAP_UINT, "dacp.fullscreenenabled" },
	{ "cafs", DMAP_UINT, "dacp.fullscreen" },
	{ "caia", DMAP_UINT, "dacp.isactive" },
	{ "cana", DMAP_STR,  "dacp.nowplayingartist" },
	{ "cang", DMAP_STR,  "dacp.nowplayinggenre" },
	{ "canl", DMAP_STR,  "dacp.nowplayingalbum" },
	{ "cann", DMAP_STR,  "dacp.nowplayingname" },
	{ "canp", DMAP_UINT, "dacp.nowplayingids" },
	{ "cant", DMAP_UINT, "dacp.nowplayingtime" },
	{ "capr", DMAP_VERS, "dacp.protocolversion" },
	{ "caps", DMAP_UINT, "dacp.playerstate" },
	{ "carp", DMAP_UINT, "dacp.repeatstate" },
	{ "cash", DMAP_UINT, "dacp.shufflestate" },
	{ "casp", DMAP_DICT, "dacp.speakers" },
	{ "cast", DMAP_UINT, "dacp.songtime" },
	{ "cavc", DMAP_UINT, "dacp.volumecontrollable" },
	{ "cave", DMAP_UINT, "dacp.visualizerenabled" },
	{ "cavs", DMAP_UINT, "dacp.visualizer" },
	{ "ceJC", DMAP_UINT, "com.apple.itunes.jukebox-client-vote" },
	{ "ceJI", DMAP_UINT, "com.apple.itunes.jukebox-current" },
	{ "ceJS", DMAP_UINT, "com.apple.itunes.jukebox-score" },
	{ "ceJV", DMAP_UINT, "com.apple.itunes.jukebox-vote" },
	{ "ceQR", DMAP_DICT, "com.apple.itunes.playqueue-contents-response" },
	{ "ceQa", DMAP_STR,  "com.apple.itunes.playqueue-album" },
	{ "ceQg", DMAP_STR,  "com.apple.itunes.playqueue-genre" },
	{ "ceQn", DMAP_STR,  "com.apple.itunes.playqueue-name" },
	{ "ceQr", DMAP_STR,  "com.apple.itunes.playqueue-artist" },
	{ "cmgt", DMAP_DICT, "dmcp.getpropertyresponse" },
	{ "cmmk", DMAP_UINT, "dmcp.mediakind" },
	{ "cmpr", DMAP_VERS, "dmcp.protocolversion" },
	{ "cmsr", DMAP_UINT, "dmcp.serverrevision" },
	{ "cmst", DMAP_DICT, "dmcp.playstatus" },
	{ "cmvo", DMAP_UINT, "dmcp.volume" },
	{ "mbcl", DMAP_DICT, "dmap.bag" },
	{ "mccr", DMAP_DICT, "dmap.contentcodesresponse" },
	{ "mcna", DMAP_STR,  "dmap.contentcodesname" },
	{ "mcnm", DMAP_UINT, "dmap.contentcodesnumber" },
	{ "mcon", DMAP_DICT, "dmap.container" },
	{ "mctc", DMAP_UINT, "dmap.containercount" },
	{ "mcti", DMAP_UINT, "dmap.containeritemid" },
	{ "mcty", DMAP_UINT, "dmap.contentcodestype" },
	{ "mdbk", DMAP_UINT, "dmap.databasekind" },
	{ "mdcl", DMAP_DICT, "dmap.dictionary" },
	{ "mdst", DMAP_UINT, "dmap.downloadstatus" },
	{ "meds", DMAP_UINT, "dmap.editcommandssupported" },
	{ "miid", DMAP_UINT, "dmap.itemid" },
	{ "mikd", DMAP_UINT, "dmap.itemkind" },
	{ "mimc", DMAP_UINT, "dmap.itemcount" },
	{ "minm", DMAP_STR,  "dmap.itemname" },
	{ "minm", DMAP_STR,  "dmap.itemname" },
	{ "mlcl", DMAP_DICT, "dmap.listing" },
	{ "mlid", DMAP_UINT, "dmap.sessionid" },
	{ "mlit", DMAP_DICT, "dmap.listingitem" },
	{ "mlog", DMAP_DICT, "dmap.loginresponse" },
	{ "mpco", DMAP_UINT, "dmap.parentcontainerid" },
	{ "mper", DMAP_UINT, "dmap.persistentid" },
	{ "mpro", DMAP_VERS, "dmap.protocolversion" },
	{ "mrco", DMAP_UINT, "dmap.returnedcount" },
	{ "mrpr", DMAP_UINT, "dmap.remotepersistentid" },
	{ "msal", DMAP_UINT, "dmap.supportsautologout" },
	{ "msas", DMAP_UINT, "dmap.authenticationschemes" },
	{ "msau", DMAP_UINT, "dmap.authenticationmethod" },
	{ "msbr", DMAP_UINT, "dmap.supportsbrowse" },
	{ "msdc", DMAP_UINT, "dmap.databasescount" },
	{ "msex", DMAP_UINT, "dmap.supportsextensions" },
	{ "msix", DMAP_UINT, "dmap.supportsindex" },
	{ "mslr", DMAP_UINT, "dmap.loginrequired" },
	{ "msma", DMAP_UINT, "dmap.machineaddress" },
	{ "msml", DMAP_DICT, "msml" },
	{ "mspi", DMAP_UINT, "dmap.supportspersistentids" },
	{ "msqy", DMAP_UINT, "dmap.supportsquery" },
	{ "msrs", DMAP_UINT, "dmap.supportsresolve" },
	{ "msrv", DMAP_DICT, "dmap.serverinforesponse" },
	{ "mstc", DMAP_DATE, "dmap.utctime" },
	{ "mstm", DMAP_UINT, "dmap.timeoutinterval" },
	{ "msto", DMAP_INT,  "dmap.utcoffset" },
	{ "msts", DMAP_STR,  "dmap.statusstring" },
	{ "mstt", DMAP_UINT, "dmap.status" },
	{ "msup", DMAP_UINT, "dmap.supportsupdate" },
	{ "mtco", DMAP_UINT, "dmap.specifiedtotalcount" },
	{ "mudl", DMAP_DICT, "dmap.deletedidlisting" },
	{ "mupd", DMAP_DICT, "dmap.updateresponse" },
	{ "musr", DMAP_UINT, "dmap.serverrevision" },
	{ "muty", DMAP_UINT, "dmap.updatetype" },
	{ "ppro", DMAP_VERS, "ppro" }
};
static const size_t dmap_type_count = sizeof(dmap_types) / sizeof(dmap_type);


typedef int (*sort_func) (const void *, const void *);


static int dmap_type_sort(const dmap_type *a, const dmap_type *b) {
	return strncmp(a->code, b->code, 4);
}


static const dmap_type *dmap_type_from_code(const char *code) {
	dmap_type key;
	key.code = code;
	return (const dmap_type *)bsearch(&key, dmap_types, dmap_type_count, sizeof(dmap_type), (sort_func)dmap_type_sort);
}


const char *CDmapParser::dmap_name_from_code(const char *code) {
	const dmap_type *t = dmap_type_from_code(code);
	return t != 0 ? t->name : 0;
}


static int16_t dmap_read_i16(const char *buf) {
	return (int16_t)((buf[0] & 0xff) <<  8) |
	((buf[1] & 0xff));
}


static uint16_t dmap_read_u16(const char *buf) {
	return (uint16_t)((buf[0] & 0xff) <<  8) |
	((buf[1] & 0xff));
}


static int32_t dmap_read_i32(const char *buf) {
	return ((int32_t)(buf[0] & 0xff) << 24) |
	((int32_t)(buf[1] & 0xff) << 16) |
	((int32_t)(buf[2] & 0xff) <<  8) |
	((int32_t)(buf[3] & 0xff));
}


static uint32_t dmap_read_u32(const char *buf) {
	return ((uint32_t)(buf[0] & 0xff) << 24) |
	((uint32_t)(buf[1] & 0xff) << 16) |
	((uint32_t)(buf[2] & 0xff) <<  8) |
	((uint32_t)(buf[3] & 0xff));
}


static int64_t dmap_read_i64(const char *buf) {
	return ((int64_t)(buf[0] & 0xff) << 56) |
	((int64_t)(buf[1] & 0xff) << 48) |
	((int64_t)(buf[2] & 0xff) << 40) |
	((int64_t)(buf[3] & 0xff) << 32) |
	((int64_t)(buf[4] & 0xff) << 24) |
	((int64_t)(buf[5] & 0xff) << 16) |
	((int64_t)(buf[6] & 0xff) <<  8) |
	((int64_t)(buf[7] & 0xff));
}


static uint64_t dmap_read_u64(const char *buf) {
	return ((uint64_t)(buf[0] & 0xff) << 56) |
	((uint64_t)(buf[1] & 0xff) << 48) |
	((uint64_t)(buf[2] & 0xff) << 40) |
	((uint64_t)(buf[3] & 0xff) << 32) |
	((uint64_t)(buf[4] & 0xff) << 24) |
	((uint64_t)(buf[5] & 0xff) << 16) |
	((uint64_t)(buf[6] & 0xff) <<  8) |
	((uint64_t)(buf[7] & 0xff));
}

CDmapParser::CDmapParser(void)
{
}


CDmapParser::~CDmapParser(void)
{
}


int CDmapParser::dmap_parse(void* ctx, const char *buf, size_t len) {
	const dmap_type *t;
	DMAP_FIELD field_type;
	size_t field_len;
	const char *field_name;
	const char *p = buf;
	const char *end = buf + len;
	char code[5];
	code[4] = '\0';


	while (end - p >= 8) {
		strncpy_s(code, sizeof(code), p, 4);
		t = dmap_type_from_code(code);
		p += 4;


		field_len = dmap_read_u32(p);
		p += 4;


		if (p + field_len > end)
			return -1;


		if (t) {
			field_type = t->type;
			field_name = t->name;
		} else {
			/* Make a best guess of the type */
			field_type = DMAP_UNKNOWN;
			field_name = code;


			if (field_len >= 8) {
				/* Look for a four char code followed by a length within the current field */
				if (isalpha(p[0]) && isalpha(p[1]) && isalpha(p[2]) && isalpha(p[3])) {
					if (dmap_read_u32(p + 4) < field_len)
						field_type = DMAP_DICT;
				}
			}


			if (field_type == DMAP_UNKNOWN) {
				size_t i;
				int is_string = 1;
				for (i=0; i < field_len; i++) {
					if (!isascii(p[i]) || p[i] < 2) {
						is_string = 0;
						break;
					}
				}


				field_type = is_string ? DMAP_STR : DMAP_UINT;
			}
		}


		switch (field_type) {
			case DMAP_UINT:
				/* Determine the integer's type based on its size */
				switch (field_len) {
					case 1:
						on_uint32(ctx, code, field_name, *p);
						break;
					case 2:
						on_uint32(ctx, code, field_name, dmap_read_u16(p));
						break;
					case 4:
						on_uint32(ctx, code, field_name, dmap_read_u32(p));
						break;
					case 8:
						on_uint64(ctx, code, field_name, dmap_read_u64(p));
						break;
					default:
						on_data(ctx, code, field_name, p, field_len);
						break;
				}
				break;
			case DMAP_INT:
				switch (field_len) {
					case 1:
						on_int32(ctx, code, field_name, *p);
						break;
					case 2:
						on_int32(ctx, code, field_name, dmap_read_i16(p));
						break;
					case 4:
						on_int32(ctx, code, field_name, dmap_read_i32(p));
						break;
					case 8:
						on_int64(ctx, code, field_name, dmap_read_i64(p));
						break;
					default:
						on_data(ctx, code, field_name, p, field_len);
						break;
				}
				break;
			case DMAP_STR:
				on_string(ctx, code, field_name, p, field_len);
				break;
			case DMAP_DATE:
				/* Seconds since epoch */
				on_date(ctx, code, field_name, dmap_read_u32(p));
				break;
			case DMAP_VERS:
				if (field_len >= 4) {
					char version[20];
					sprintf_s(version, 20, "%d.%d", dmap_read_u16(p), dmap_read_u16(p+2));
					on_string(ctx, code, field_name, version, strlen(version));
				}
				break;
			case DMAP_DICT:
				on_dict_start(ctx, code, field_name);
				if (dmap_parse(ctx, p, field_len) != 0)
					return -1;
				on_dict_end(ctx, code, field_name);
				break;
			case DMAP_UNKNOWN:
				break;
		}


		p += field_len;
	}


	if (p != end)
		return -1;


	return 0;
}

