/* gpsd.h -- fundamental types and structures for the gpsd library
 *
 * This file is Copyright (c) 2010 by the GPSD project
 * BSD terms apply: see the file COPYING in the distribution root for details.
 */

#ifndef _GPSD_H_
#define _GPSD_H_

# ifdef __cplusplus
extern "C" {
# endif

#include <stdbool.h>
#include <stdio.h>

#include <termios.h>
#include <stdint.h>
#include <stdarg.h>
#include "gps.h"
#include "gpsd_config.h"

/*
 * Tell GCC that we want thread-safe behavior with _REENTRANT;
 * in particular, errno must be thread-local.
 * Tell POSIX-conforming implementations with _POSIX_THREAD_SAFE_FUNCTIONS.
 * See http://www.unix.org/whitepapers/reentrant.html
 */
#ifndef _REENTRANT
#define _REENTRANT
#endif
#ifndef _POSIX_THREAD_SAFE_FUNCTIONS
#define _POSIX_THREAD_SAFE_FUNCTIONS
#endif

/* use RFC 2782 PPS API */
/* this needs linux >= 2.6.34 and
 * CONFIG_PPS=y
 * CONFIG_PPS_DEBUG=y  [optional to kernel log pulses]
 * CONFIG_PPS_CLIENT_LDISC=y
 */
#ifndef S_SPLINT_S
#if defined(HAVE_SYS_TIMEPPS_H)
// include unistd.h here as it is missing on older pps-tools releases.
// 'close' is not defined otherwise.
#include <unistd.h>
#include <sys/time.h>
#include <sys/timepps.h>
#endif /* S_SPLINT_S */
#endif

#ifdef _WIN32
typedef unsigned int speed_t;
#endif

/*
 * Constants for the VERSION response
 * 3.1: Base JSON version
 * 3.2: Added POLL command and response
 * 3.3: AIS app_id split into DAC and FID
 * 3.4: Timestamps change from seconds since Unix epoch to ISO8601.
 * 3.5: POLL subobject name changes: fixes -> tpv, skyview -> sky.
 *      DEVICE::activated becomes ISO8601 rather than real.
 * 3.6  VERSION, WATCH, and DEVICES from slave gpsds get "remote" attribute.
 * 3.7  PPS message added to repertoire. SDDBT water depth reported as
 *      negative altitude with Mode 3 set.
 * 3.8  AIS course member becomes float in scaled mode (bug fix).
 * 3.9  split24 flag added. Controlled-vocabulary fields are now always
 *      dumped in both numeric and string form, with the string being the
 *      value of a synthesized additional attribute with "_text" appended.
 *      (Thus, the 'scaled' flag no longer affects display of these fields.)
 *      PPS drift message ships nsec rather than msec.
 */
#define GPSD_PROTO_MAJOR_VERSION	3	/* bump on incompatible changes */
#define GPSD_PROTO_MINOR_VERSION	9	/* bump on compatible changes */

#define JSON_DATE_MAX	24	/* ISO8601 timestamp with 2 decimal places */

#ifndef DEFAULT_GPSD_SOCKET
#define DEFAULT_GPSD_SOCKET	"/var/run/gpsd.sock"
#endif 

/* Some internal capabilities depend on which drivers we're compiling. */
#if !defined(NMEA_ENABLE) && (defined(FV18_ENABLE) || defined(MTK3301_ENABLE) || defined(TNT_ENABLE) || defined(OCEANSERVER_ENABLE) || defined(GPSCLOCK_ENABLE) || defined(FURY_ENABLE))
#define NMEA_ENABLE
#endif
#ifdef EARTHMATE_ENABLE
#define ZODIAC_ENABLE
#endif
#if defined(ZODIAC_ENABLE) || defined(SIRF_ENABLE) || defined(GARMIN_ENABLE) || defined(TSIP_ENABLE) || defined(EVERMORE_ENABLE) || defined(ITRAX_ENABLE) || defined(UBLOX_ENABLE) || defined(SUPERSTAR2_ENABLE) || defined(ONCORE_ENABLE) || defined(GEOSTAR_ENABLE) || defined(NAVCOM_ENABLE) || defined(NMEA2000_ENABLE) || defined(SEATALK_ENABLE)
#define BINARY_ENABLE
#endif
#if defined(TRIPMATE_ENABLE) || defined(BINARY_ENABLE)
#define NON_NMEA_ENABLE
#endif
#if defined(TNT_ENABLE) || defined(OCEANSERVER_ENABLE)
#define COMPASS_ENABLE
#endif
#ifdef NTPSHM_ENABLE
#define TIMEHINT_ENABLE
#endif

/* First, declarations for the packet layer... */

/*
 * For NMEA-conforming receivers this is supposed to be 82, but
 * some receivers (TN-200, GSW 2.3.2) emit oversized sentences.
 * The current hog champion is the Trimble BX-960 receiver, which
 * emits a 91-character GGA message.
 */
#define NMEA_MAX	91		/* max length of NMEA sentence */
#define NMEA_BIG_BUF	(2*NMEA_MAX+1)	/* longer than longest NMEA sentence */

/* a few bits of ISGPS magic */
enum isgpsstat_t {
    ISGPS_NO_SYNC, ISGPS_SYNC, ISGPS_SKIP, ISGPS_MESSAGE,
};

#define RTCM_MAX	(RTCM2_WORDS_MAX * sizeof(isgps30bits_t))

/*
 * The packet buffers need to be as long than the longest packet we
 * expect to see in any protocol, because we have to be able to hold
 * an entire packet for checksumming...
 * First we thought it had to be big enough for a SiRF Measured Tracker
 * Data packet (188 bytes). Then it had to be big enough for a UBX SVINFO
 * packet (206 bytes). Now it turns out that a couple of ITALK messages are
 * over 512 bytes. I know we like verbose output, but this is ridiculous.
 */
#define MAX_PACKET_LENGTH	516	/* 7 + 506 + 3 */

/*
 * UTC of second 0 of week 0 of the first rollover period of GPS time.
 * Used to compute UTC from GPS time. Also, the threshold value
 * under which system clock times are considered unreliable. Often,
 * embedded systems come up thinking it's early 1970 and the system
 * clock will report small positive values until the clock is set.  By
 * choosing this as the cutoff, we'll never reject historical GPS logs
 * that are actually valid.
 */
#define GPS_EPOCH	315964800	/* 6 Jan 1981 00:00:00 UTC */

/* time constant */
#define SECS_PER_DAY	(60*60*24)		/* seconds per day */
#define SECS_PER_WEEK	(7*SECS_PER_DAY)	/* seconds per week */
#define GPS_ROLLOVER	(1024*SECS_PER_WEEK)	/* rollover period */

struct gps_packet_t {
    /* packet-getter internals */
    int	type;
#define BAD_PACKET      	-1
#define COMMENT_PACKET  	0
#define NMEA_PACKET     	1
#define AIVDM_PACKET    	2
#define GARMINTXT_PACKET	3
#define MAX_TEXTUAL_TYPE	3	/* increment this as necessary */
#define SIRF_PACKET     	4
#define ZODIAC_PACKET   	5
#define TSIP_PACKET     	6
#define EVERMORE_PACKET 	7
#define ITALK_PACKET    	8
#define GARMIN_PACKET   	9
#define NAVCOM_PACKET   	10
#define UBX_PACKET      	11
#define SUPERSTAR2_PACKET	12
#define ONCORE_PACKET   	13
#define GEOSTAR_PACKET   	14
#define NMEA2000_PACKET 	15
#define VYSPI_PACKET 	        16
#define SEATALK_PACKET 	        17
#define MAX_GPSPACKET_TYPE	17	/* increment this as necessary */
#define RTCM2_PACKET    	18
#define RTCM3_PACKET    	19
#define JSON_PACKET    	    	20
#define TEXTUAL_PACKET_TYPE(n)	((((n)>=NMEA_PACKET) && ((n)<=MAX_TEXTUAL_TYPE)) || (n)==JSON_PACKET)
#define GPS_PACKET_TYPE(n)	(((n)>=NMEA_PACKET) && ((n)<=MAX_GPSPACKET_TYPE))
#define LOSSLESS_PACKET_TYPE(n)	(((n)>=RTCM2_PACKET) && ((n)<=RTCM3_PACKET))
#define PACKET_TYPEMASK(n)	(1 << (n))
#define GPS_TYPEMASK	(((2<<(MAX_GPSPACKET_TYPE+1))-1) &~ PACKET_TYPEMASK(COMMENT_PACKET))


    unsigned int frm_type;
    unsigned int frm_state;
    unsigned int frm_7dflag;
    unsigned int frm_offset;
    unsigned int frm_length;
    unsigned int frm_read;
    unsigned int frm_version;
    unsigned int frm_port;

    unsigned int frm_reserved;

    unsigned int frm_act_checksum;
    unsigned int frm_shall_checksum;

    unsigned int state;
    size_t length;
    unsigned char inbuffer[MAX_PACKET_LENGTH*2+1];
    size_t inbuflen;
    unsigned /*@observer@*/char *inbufptr;
    /* outbuffer needs to be able to hold 4 GPGSV records at once */

#define MAX_OUT_BUF_RECORDS 312 // something safe above MAX_PACKET_LENGTH*2+1 / 3
    uint16_t   out_count;
    uint8_t   out_type[MAX_OUT_BUF_RECORDS];
    uint8_t   out_new_version[MAX_OUT_BUF_RECORDS];
    uint16_t  out_offset[MAX_OUT_BUF_RECORDS];
    uint16_t  out_len[MAX_OUT_BUF_RECORDS];
    uint8_t   outbuffer[MAX_PACKET_LENGTH*2+1];
    size_t outbuflen;
    unsigned long char_counter;		/* count characters processed */
    unsigned long retry_counter;	/* count sniff retries */
    unsigned counter;			/* packets since last driver switch */
    int debug;				/* lexer debug level */
#ifdef TIMING_ENABLE
    timestamp_t start_time;		/* timestamp of first input */
    unsigned long start_char;		/* char counter at first input */
#endif /* TIMING_ENABLE */
    /*
     * ISGPS200 decoding context.
     *
     * This is not conditionalized on RTCM104_ENABLE because we need to
     * be able to build gpsdecode even when RTCM support is not
     * configured in the daemon.
     */
    struct {
	bool            locked;
	int             curr_offset;
	isgps30bits_t   curr_word;
	unsigned int    bufindex;
	/*
	 * Only these should be referenced from elsewhere, and only when
	 * RTCM_MESSAGE has just been returned.
	 */
	isgps30bits_t   buf[RTCM2_WORDS_MAX];   /* packet data */
	size_t          buflen;                 /* packet length in bytes */
    } isgps;
#ifdef PASSTHROUGH_ENABLE
    unsigned int json_depth;
    unsigned int json_after;
#endif /* PASSTHROUGH_ENABLE */
};

extern void packet_init(/*@out@*/struct gps_packet_t *);
extern void packet_reset(/*@out@*/struct gps_packet_t *);
extern void packet_pushback(struct gps_packet_t *);
extern void packet_parse(struct gps_packet_t *);
extern ssize_t packet_get(int, struct gps_packet_t *);
extern int packet_sniff(struct gps_packet_t *);
#define packet_buffered_input(lexer) ((lexer)->inbuffer + (lexer)->inbuflen - (lexer)->inbufptr)

/* Next, declarations for the core library... */

/* factors for converting among confidence interval units */
#define CEP50_SIGMA	1.18
#define DRMS_SIGMA	1.414
#define CEP95_SIGMA	2.45

/* this is where we choose the confidence level to use in reports */
#define GPSD_CONFIDENCE	CEP95_SIGMA

#define NTPSHMSEGS	4		/* number of NTP SHM segments */

#define AIVDM_CHANNELS	2		/* A, B */

struct gps_device_t;

struct gps_context_t {
    int valid;				/* member validity flags */
#define LEAP_SECOND_VALID	0x01	/* we have or don't need correction */
#define GPS_TIME_VALID  	0x02	/* GPS week/tow is valid */
#define CENTURY_VALID		0x04	/* have received ZDA or 4-digit year */
    int debug;				/* dehug verbosity level */
    bool readonly;			/* if true, never write to device */
    /* DGPS status */
    int fixcnt;				/* count of good fixes seen */
    /* timekeeping */
    time_t start_time;			/* local time of daemon startup */
    int leap_seconds;			/* Unix seconds to UTC (GPS-UTC offset) */
    unsigned short gps_week;            /* GPS week, actually 10 bits */
    double gps_tow;                     /* GPS time of week, actually 19 bits */
    int century;			/* for NMEA-only devices without ZDA */
    int rollovers;			/* rollovers since start of run */
#ifdef TIMEHINT_ENABLE
    int leap_notify;			/* notification state from subframe */
#define LEAP_NOWARNING  0x0     /* normal, no leap second warning */
#define LEAP_ADDSECOND  0x1     /* last minute of day has 60 seconds */
#define LEAP_DELSECOND  0x2     /* last minute of day has 59 seconds */
#define LEAP_NOTINSYNC  0x3     /* overload, clock is free running */
#endif /* TIMEHINT_ENABLE */
#ifdef NTPSHM_ENABLE
    /* we need the volatile here to tell the C compiler not to
     * 'optimize' as 'dead code' the writes to SHM */
    /*@reldef@*/volatile struct shmTime *shmTime[NTPSHMSEGS];
    bool shmTimeInuse[NTPSHMSEGS];
#endif /* NTPSHM_ENABLE */
#ifdef PPS_ENABLE
    /*@null@*/ void (*pps_hook)(struct gps_device_t *, struct timedrift_t *);
#endif /* PPS_ENABLE */
#ifdef SHM_EXPORT_ENABLE
    /* we don't want the compiler to treat writes to shmexport as dead code,
     * and we don't want them reordered either */
    /*@reldef@*/volatile char *shmexport;
#endif
};

/* state for resolving interleaved Type 24 packets */
struct ais_type24a_t {
    unsigned int mmsi;
    char shipname[AIS_SHIPNAME_MAXLEN+1];
};
#define MAX_TYPE24_INTERLEAVE	8	/* max number of queued type 24s */
struct ais_type24_queue_t {
    struct ais_type24a_t ships[MAX_TYPE24_INTERLEAVE];
    int index;
};

/* state for resolving AIVDM decodes */
struct aivdm_context_t {
    /* hold context for decoding AIDVM packet sequences */
    int decoded_frags;		/* for tracking AIDVM parts in a multipart sequence */
    unsigned char bits[2048];
    size_t bitlen; /* how many valid bits */
    struct ais_type24_queue_t type24_queue;
};

#define MODE_NMEA	0
#define MODE_BINARY	1

typedef enum {ANY, GPS, RTCM2, RTCM3, AIS} gnss_type;
typedef enum {
    event_wakeup,
    event_triggermatch,
    event_identified,
    event_configure,
    event_driver_switch,
    event_deactivate,
    event_reactivate,
} event_t;


#define INTERNAL_SET(n)	((gps_mask_t)(1llu<<(SET_HIGH_BIT+(n))))
#define RAW_IS  	INTERNAL_SET(1)	/* raw pseudorange data available */
#define USED_IS 	INTERNAL_SET(2)	/* sat-used count available */
#define DRIVER_IS	INTERNAL_SET(3)	/* driver type identified */
#define CLEAR_IS	INTERNAL_SET(4)	/* starts a reporting cycle */
#define REPORT_IS	INTERNAL_SET(5)	/* ends a reporting cycle */
#define NODATA_IS	INTERNAL_SET(6)	/* no data read from fd */
#define PPSTIME_IS	INTERNAL_SET(7)	/* precision time is available */
#define PERR_IS 	INTERNAL_SET(8)	/* PDOP set */
#define PASSTHROUGH_IS 	INTERNAL_SET(9)	/* passthrough mode */
#define DATA_IS	~(ONLINE_SET|PACKET_SET|CLEAR_IS|REPORT_IS)

typedef /*@unsignedintegraltype@*/ unsigned int driver_mask_t;
#define DRIVER_NOFLAGS	0x00000000u
#define DRIVER_STICKY	0x00000001u

/*
 * True if a device type is non-null and has control methods.
 */
#define CONTROLLABLE(dp)	(((dp) != NULL) && \
				 ((dp)->speed_switcher != NULL		\
				  || (dp)->mode_switcher != NULL	\
				  || (dp)->rate_switcher != NULL))

/*
 * True if a driver selection of it should be sticky. 
 */
#define STICKY(dp)		((dp) != NULL && ((dp)->flags & DRIVER_STICKY) != 0)

struct gps_type_t {
/* GPS method table, describes how to talk to a particular GPS type */
    /*@observer@*/const char *type_name;
    int packet_type;
    driver_mask_t flags;	/* reserved for expansion */
    /*@observer@*//*@null@*/const char *trigger;
    int channels;
    /*@null@*/bool (*probe_detect)(struct gps_device_t *session);
    /*@null@*/ssize_t (*get_packet)(struct gps_device_t *session);
    /*@null@*/gps_mask_t (*parse_packet)(struct gps_device_t *session);
    /*@null@*/ssize_t (*rtcm_writer)(struct gps_device_t *session, const uint8_t *rtcmbuf, size_t rtcmbytes);
    /*@null@*/void (*event_hook)(struct gps_device_t *session, event_t event);
#ifdef RECONFIGURE_ENABLE
    /*@null@*/bool (*speed_switcher)(struct gps_device_t *session,
				     speed_t speed, char parity, int stopbits);
    /*@null@*/void (*mode_switcher)(struct gps_device_t *session, int mode);
    /*@null@*/bool (*rate_switcher)(struct gps_device_t *session, double rate);
    double min_cycle;
#endif /* RECONFIGURE_ENABLE */
#ifdef CONTROLSEND_ENABLE
    /*@null@*/ssize_t (*control_send)(struct gps_device_t *session, char *buf, size_t buflen);
#endif /* CONTROLSEND_ENABLE */
#ifdef TIMEHINT_ENABLE
    /*@null@*/double (*time_offset)(struct gps_device_t *session);
#endif /* TIMEHINT_ENABLE */
};

/*
 * Each input source has an associated type.  This is currently used in two
 * ways:
 *
 * (1) To determince if we require that gpsd be the only process opening a
 * device.  We make an exception for PTYs because the master side has to be
 * opened by test code,
 *
 * (2) To determine whether it's safe to send wakeup strings.  These are
 * required on some unusual RS-232 devices (such as the TNT compass and
 * Thales/Ashtech GPSes) but should not be shipped to unidentified USB
 * or Bluetooth devices as we don't even know in advance those are GPSEs;
 * they might not cope well.
 *
 * Where it says "case detected but not used" it means that we can identify
 * a source type but no behavior is yet contingent on it.  A "discoverable"
 * device is one for which there is discoverable metadata such as a
 * vendor/product ID.
 *
 * We should never see a block device; that would indicate a serious error
 * in command-line usage or the hotplug system.
 */
typedef enum {source_unknown,
	      source_blockdev,	/* block devices can't be GPS sources */
	      source_rs232,	/* potential GPS source, not discoverable */
	      source_usb,	/* potential GPS source, discoverable */
	      source_bluetooth,	/* potential GPS source, discoverable */
	      source_can,	/* potential GPS source, fixed CAN format */
	      source_pty,	/* PTY: we don't require exclusive access */
	      source_tcp,	/* TCP/IP stream: case detected but not used */
	      source_udp,	/* UDP stream: case detected but not used */
	      source_gpsd,	/* Remote gpsd instance over TCP/IP */
} sourcetype_t;

/*
 * Each input source also has an associated service type.
 */
typedef enum {service_unknown,
	      service_sensor,
	      service_dgpsip,
	      service_ntrip,
} servicetype_t;

/*
 * Private state information about an NTRIP stream.
 */
enum ntrip_stream_format_t
{
    fmt_rtcm2,
    fmt_rtcm2_0,
    fmt_rtcm2_1,
    fmt_rtcm2_2,
    fmt_rtcm2_3,
    fmt_rtcm3,
    fmt_unknown
};

enum ntrip_stream_compr_encryp_t
{ cmp_enc_none, cmp_enc_unknown };

enum ntrip_stream_authentication_t
{ auth_none, auth_basic, auth_digest, auth_unknown };

struct ntrip_stream_t
{
    char mountpoint[101];
    char credentials[128];
    char authStr[128];
    char url[256];
    char port[32]; /* in my /etc/services 16 was the longest */
    bool set; /* found and set */
    enum ntrip_stream_format_t format;
    int carrier;
    double latitude;
    double longitude;
    int nmea;
    enum ntrip_stream_compr_encryp_t compr_encryp;
    enum ntrip_stream_authentication_t authentication;
    int fee;
    int bitrate;
};

/*
 * This hackery is intended to support SBCs that are resource-limited
 * and only need to support one or a few devices each.  It avoids the
 * space overhead of allocating thousands of unused device structures.
 * This array fills from the bottom, so as an extreme case you could
 * reduce LIMITED_MAX_DEVICES to 1.
 */
#ifdef LIMITED_MAX_DEVICES
#define MAXDEVICES	LIMITED_MAX_DEVICES
#else
/* we used to make this FD_SETSIZE, but that cost 14MB of wasted core! */
#define MAXDEVICES	4
#endif

#define sub_index(s) (int)((s) - subscribers)
#define allocated_device(devp)	 ((devp)->gpsdata.dev.path[0] != '\0')
#define free_device(devp)	 (devp)->gpsdata.dev.path[0] = '\0'
#define initialized_device(devp) ((devp)->context != NULL)

/* state information about our response parsing */
enum ntrip_conn_state_t {
    ntrip_conn_init,
    ntrip_conn_sent_probe,
    ntrip_conn_sent_get,
    ntrip_conn_established,
    ntrip_conn_err
}; 	/* connection state for multi stage connect */

struct gps_device_t {
/* session object, encapsulates all global state */
    struct gps_data_t gpsdata;
    struct data_central_t * data_central;

    /*@relnull@*/const struct gps_type_t *device_type;
    unsigned int driver_index;		/* numeric index of current driver */
    unsigned int drivers_identified;	/* bitmask; what drivers have we seen? */
#ifdef RECONFIGURE_ENABLE
    /*@relnull@*/const struct gps_type_t *last_controller;
#endif /* RECONFIGURE_ENABLE */
    struct gps_context_t	*context;
    sourcetype_t sourcetype;
    servicetype_t servicetype;
    int mode;
#ifndef _WIN32
    struct termios ttyset, ttyset_old;
#endif
#ifndef FIXED_PORT_SPEED
    unsigned int baudindex;
#endif /* FIXED_PORT_SPEED */
    int saved_baud;
    struct gps_packet_t packet;
    int badcount;
    int subframe_count;
    char subtype[64];			/* firmware version or subtype ID */
    timestamp_t opentime;
    timestamp_t releasetime;
    bool zerokill;
    timestamp_t reawake;
#ifdef TIMING_ENABLE
    timestamp_t sor;	/* timestamp start of this reporting cycle */
    unsigned long chars;	/* characters in the cycle */
#endif /* TIMING_ENABLE */
#ifdef NTPSHM_ENABLE
    bool ship_to_ntpd;
    int shmIndex;
# ifdef PPS_ENABLE
    int shmIndexPPS;
# endif /* PPS_ENABLE */
#endif /* NTPSHM_ENABLE */
    volatile struct {
	timestamp_t real;
	timestamp_t clock;
    } last_fixtime;	/* so updates happen once */
#ifdef PPS_ENABLE
#if defined(HAVE_SYS_TIMEPPS_H)
    pps_handle_t kernelpps_handle;
#endif /* defined(HAVE_SYS_TIMEPPS_H) */
    int chronyfd;			/* for talking to chrony */
    /*@null@*/ char *(*thread_report_hook)(struct gps_device_t *,
					   struct timedrift_t *);
    /*@null@*/ void (*thread_wrap_hook)(struct gps_device_t *);
    struct timedrift_t ppslast;
    int ppscount;
#endif /* PPS_ENABLE */
    double mag_var;			/* magnetic variation in degrees */
    bool back_to_nmea;			/* back to NMEA on revert? */
    char msgbuf[MAX_PACKET_LENGTH*2+1];	/* command message buffer for sends */
    size_t msgbuflen;
    int observed;			/* which packet type`s have we seen? */
    bool cycle_end_reliable;		/* does driver signal REPORT_MASK */
    int fixcnt;				/* count of fixes from this device */
    struct gps_fix_t newdata;		/* where drivers put their data */
    struct gps_fix_t oldfix;		/* previous fix for error modeling */
    /*
     * The rest of this structure is driver-specific private storage.
     * It used to be a union, but that turned out to be unsafe.  Dual-mode
     * devices like SiRFs and u-bloxes need to not step on the old mode's
     * storage when they transition.
     */
    struct {
#ifdef NMEA_ENABLE
	struct {
	    int part, await;		/* for tracking GSV parts */
	    struct tm date;		/* date part of last sentence time */
	    double subseconds;		/* subsec part of last sentence time */
	    char *field[NMEA_MAX];
	    unsigned char fieldcopy[NMEA_MAX+1];
	    /* detect receivers that ship GGA with non-advancing timestamp */
	    bool latch_mode;
	    char last_gga_timestamp[16];
	    /*
	     * State for the cycle-tracking machinery.
	     * The reason these timestamps are separate from the
	     * general sentence timestamps is that we can
	     * use the minutes and seconds part of a sentence
	     * with an incomplete timestamp (like GGA) for
	     * end-cycle recognition, even if we don't have a previous
	     * RMC or ZDA that lets us get full time from it.
	     */
	    timestamp_t this_frac_time, last_frac_time;
	    bool latch_frac_time;
	    unsigned int lasttag;
	    unsigned int cycle_enders;
	    bool cycle_continue;
#ifdef GPSCLOCK_ENABLE
	    bool ignore_trailing_edge;
#endif /* GPSCLOCK_ENABLE */
	} nmea;
#endif /* NMEA_ENABLE */
#ifdef GARMINTXT_ENABLE
	struct {
	    struct tm date;		/* date part of last sentence time */
	    double subseconds;		/* subsec part of last sentence time */
	} garmintxt;
#endif /* NMEA_ENABLE */
#ifdef BINARY_ENABLE
#ifdef GEOSTAR_ENABLE
	struct {
	    unsigned int physical_port;
	} geostar;
#endif /* GEOSTAR_ENABLE */
#ifdef SIRF_ENABLE
	struct {
	    unsigned int need_ack;	/* if NZ we're awaiting ACK */
	    unsigned int cfg_stage;	/* configuration stage counter */
	    unsigned int driverstate;	/* for private use */
#define SIRF_LT_231	0x01		/* SiRF at firmware rev < 231 */
#define SIRF_EQ_231     0x02            /* SiRF at firmware rev == 231 */
#define SIRF_GE_232     0x04            /* SiRF at firmware rev >= 232 */
#define UBLOX   	0x08		/* u-blox firmware with packet 0x62 */
	    unsigned long satcounter;
	    unsigned int time_seen;
#define TIME_SEEN_UTC_2	0x08	/* Seen UTC time variant 2? */
	    /* fields from Navigation Parameters message */
	    bool nav_parameters_seen;	/* have we seen one? */
	    unsigned char altitude_hold_mode;
	    unsigned char altitude_hold_source;
	    int16_t altitude_source_input;
	    unsigned char degraded_mode;
	    unsigned char degraded_timeout;
	    unsigned char dr_timeout;
	    unsigned char track_smooth_mode;
	    /* fields from DGPS Status */
	    unsigned int dgps_source;
#define SIRF_DGPS_SOURCE_NONE		0 /* No DGPS correction type have been selected */
#define SIRF_DGPS_SOURCE_SBAS		1 /* SBAS */
#define SIRF_DGPS_SOURCE_SERIAL		2 /* RTCM corrections */
#define SIRF_DGPS_SOURCE_BEACON		3 /* Beacon corrections */
#define SIRF_DGPS_SOURCE_SOFTWARE	4 /*  Software API corrections */
	} sirf;
#endif /* SIRF_ENABLE */
#ifdef SUPERSTAR2_ENABLE
	struct {
	    time_t last_iono;
	} superstar2;
#endif /* SUPERSTAR2_ENABLE */
#ifdef TSIP_ENABLE
	struct {
	    bool superpkt;		/* Super Packet mode requested */
	    time_t last_41;		/* Timestamps for packet requests */
	    time_t last_48;
	    time_t last_5c;
	    time_t last_6d;
	    time_t last_46;
	    time_t req_compact;
	    unsigned int stopbits; /* saved RS232 link parameter */
	    char parity;
	    int subtype;
#define TSIP_UNKNOWN    	0
#define TSIP_ACCUTIME_GOLD	1
	} tsip;
#endif /* TSIP_ENABLE */
#ifdef GARMIN_ENABLE	/* private housekeeping stuff for the Garmin driver */
	struct {
	    unsigned char Buffer[4096+12];	/* Garmin packet buffer */
	    size_t BufferLen;		/* current GarminBuffer Length */
	} garmin;
#endif /* GARMIN_ENABLE */
#ifdef ZODIAC_ENABLE	/* private housekeeping stuff for the Zodiac driver */
	struct {
	    unsigned short sn;		/* packet sequence number */
	    /*
	     * Zodiac chipset channel status from PRWIZCH. Keep it so
	     * raw-mode translation of Zodiac binary protocol can send
	     * it up to the client.
	     */
#define ZODIAC_CHANNELS	12
	    unsigned int Zs[ZODIAC_CHANNELS];	/* satellite PRNs */
	    unsigned int Zv[ZODIAC_CHANNELS];	/* signal values (0-7) */
	} zodiac;
#endif /* ZODIAC_ENABLE */
#ifdef UBLOX_ENABLE
	struct {
	    unsigned char port_id;
	    unsigned char sbas_in_use;
	    /*
	     * NAV-* message order is not defined, thus we handle them isochronously
	     * and store the latest data into these variables rather than expect
	     * some messages to arrive in order. NAV-SOL handler picks up these values
	     * and inserts them into the fix structure in one go.
	     */
	    double last_herr;
	    double last_verr;
    	} ubx;
#endif /* UBLOX_ENABLE */
#ifdef NAVCOM_ENABLE
	struct {
	    uint8_t physical_port;
	    bool warned;
	} navcom;
#endif /* NAVCOM_ENABLE */
#ifdef ONCORE_ENABLE
	struct {
#define ONCORE_VISIBLE_CH 12
	    int visible;
	    int PRN[ONCORE_VISIBLE_CH];		/* PRNs of satellite */
	    int elevation[ONCORE_VISIBLE_CH];	/* elevation of satellite */
	    int azimuth[ONCORE_VISIBLE_CH];	/* azimuth */
	    int pps_offset_ns;
	} oncore;
#endif /* ONCORE_ENABLE */
#if defined(NMEA2000_ENABLE)||defined(VYSPI_ENABLE)
        struct {
            unsigned int can_msgcnt;
            unsigned int can_net;
            unsigned int unit;
            unsigned int unit_valid;
            int mode;
            unsigned int mode_valid;
            unsigned int idx;
//	    size_t ptr;
            size_t fast_packet_len;
            int type;
            void *workpgn;
            void *pgnlist;

            unsigned char sid[8];
            uint16_t manufactureid;
            uint32_t deviceid;
            uint8_t own_src_id;

            uint8_t enable_writing;

        } nmea2000;
#endif /* NMEA2000_ENABLE */
#ifdef VYSPI_ENABLE
        struct {
            uint32_t last_pgn;
            uint8_t prio;
            uint8_t src;
            uint8_t dest;
            
            uint32_t bytes_written_frm[5]; /* collecting stats per type */ 
            uint32_t bytes_written_raw[5]; /* net amount of data w/o frm overhead */
            uint32_t bytes_written_last_ms;
            uint32_t bytes_written_last_sec;
        } vyspi;
#endif /* VYSPI_ENABLE */
#ifdef SEATALK_ENABLE
	struct {
	  struct tm date;		/* date part of last sentence time */

	  /* Ugly hack for junk GPS like Raystar 112/120 only reporting
	     a timestamp (without ms!) every 10 secs. 
	     We record system time of the last reported timestamp here
	     to calculate fix.time from diff between system time and lastts */
	  timestamp_t lastts;
	  double offset;                /* offset in seconds */

	  /* This is to cope with the reporting of 
	     lat and lon in separate sentences */
	  double lat;
	  double lon;
	  int lat_set;
	  int lon_set;
	} seatalk;
#endif /* SEATALK_ENABLE */
	/*
	 * This is not conditionalized on RTCM104_ENABLE because we need to
	 * be able to build gpsdecode even when RTCM support is not
	 * configured in the daemon.  It doesn't take up extra space.
	 */
	struct {
	    /* ISGPS200 decoding */
	    bool            locked;
	    int             curr_offset;
	    isgps30bits_t   curr_word;
	    isgps30bits_t   buf[RTCM2_WORDS_MAX];
	    unsigned int    bufindex;
	} isgps;
#endif /* BINARY_ENABLE */
#ifdef AIVDM_ENABLE
	struct {
	    struct aivdm_context_t context[AIVDM_CHANNELS];
	    char ais_channel;
	} aivdm;
#endif /* AIVDM_ENABLE */
    } driver;

    /*
     * State of an NTRIP connection.  We don't want to zero this on every
     * activation, otherwise the connection state will get lost.  Information
     * in this substructure is only valid if servicetype is service_ntrip.
     */
    struct {
	/* state information about the stream */
	struct ntrip_stream_t stream;
    enum ntrip_conn_state_t conn_state;

	bool works;		/* marks a working connection, so we try to reconnect once */
	bool sourcetable_parse;	/* have we read the sourcetable header? */
    } ntrip;
    /* State of a DGPSIP connection */
    struct {
	bool reported;
    } dgpsip;
};

/* logging levels */
#define LOG_ERROR 	-1	/* errors, display always */
#define LOG_SHOUT	0	/* not an error but we should always see it */
#define LOG_WARN	1	/* not errors but may indicate a problem */
#define LOG_CLIENT	2	/* log JSON reports to clients */
#define LOG_INF 	3	/* key informative messages */
#define LOG_PROG	4	/* progress messages */
#define LOG_IO  	5	/* IO to and from devices */
#define LOG_DATA	6	/* log data management messages */
#define LOG_SPIN	7	/* logging for catching spin bugs */
#define LOG_RAW 	8	/* raw low-level I/O */

#define ISGPS_ERRLEVEL_BASE	LOG_RAW

#define IS_HIGHEST_BIT(v,m)	(v & ~((m<<1)-1))==0

/* driver helper functions */
extern void isgps_init(/*@out@*/struct gps_packet_t *);
enum isgpsstat_t isgps_decode(struct gps_packet_t *,
			      bool (*preamble_match)(isgps30bits_t *),
			      bool (*length_check)(struct gps_packet_t *),
			      size_t,
			      unsigned int);
extern unsigned int isgps_parity(isgps30bits_t);
extern void isgps_output_magnavox(const isgps30bits_t *, unsigned int, FILE *);

extern enum isgpsstat_t rtcm2_decode(struct gps_packet_t *, unsigned int);
extern void json_rtcm2_dump(const struct rtcm2_t *,
			    /*@null@*/const char *, /*@out@*/char[], size_t);
extern void rtcm2_unpack(/*@out@*/struct rtcm2_t *, char *);
extern void json_rtcm3_dump(const struct rtcm3_t *,
			    /*@null@*/const char *, /*@out@*/char[], size_t);
extern void rtcm3_unpack(const int, /*@out@*/struct rtcm3_t *, char *);

/* here are the available GPS drivers */
extern const struct gps_type_t **gpsd_drivers;

/* gpsd library internal prototypes */
extern gps_mask_t generic_parse_input(struct gps_device_t *);
extern ssize_t generic_get(struct gps_device_t *);

extern void character_discard(struct gps_packet_t *lexer);
extern void character_pushback(struct gps_packet_t *lexer);
extern void packet_discard(struct gps_packet_t *lexer);
extern void packet_accept(struct gps_packet_t *lexer, int packet_type);

extern gps_mask_t nmea_parse(char *, struct gps_device_t *);
extern gps_mask_t nmea_parse_len(char *sentence, size_t sentence_len, 
                      struct gps_device_t * session);
extern ssize_t nmea_write(struct gps_device_t *, char *, size_t);
extern ssize_t nmea_send(struct gps_device_t *, const char *, ... );
extern void nmea_add_checksum(char *);

extern gps_mask_t sirf_parse(struct gps_device_t *, unsigned char *, size_t);
extern gps_mask_t evermore_parse(struct gps_device_t *, unsigned char *, size_t);
extern gps_mask_t navcom_parse(struct gps_device_t *, unsigned char *, size_t);
extern gps_mask_t garmin_ser_parse(struct gps_device_t *);
extern gps_mask_t garmintxt_parse(struct gps_device_t *);
extern gps_mask_t aivdm_parse(struct gps_device_t *);

extern bool netgnss_uri_check(char *);
extern int netgnss_uri_open(struct gps_device_t *, char *);
extern void netgnss_report(struct gps_context_t *,
			 struct gps_device_t *,
			 struct gps_device_t *);
extern void netgnss_autoconnect(struct gps_context_t *, double, double);

extern int dgpsip_open(struct gps_device_t *, char *);
extern void dgpsip_report(struct gps_context_t *,
			 struct gps_device_t *,
			 struct gps_device_t *);
extern void dgpsip_autoconnect(struct gps_context_t *,
			       double, double, const char *);
extern int ntrip_open(struct gps_device_t *, const char *);
extern void ntrip_report(struct gps_context_t *,
			 struct gps_device_t *,
			 struct gps_device_t *);

extern void gpsd_tty_init(struct gps_device_t *);
extern int gpsd_serial_open(struct gps_device_t *);
extern bool gpsd_set_raw(struct gps_device_t *);
extern ssize_t gpsd_serial_write(struct gps_device_t *,
				 const uint8_t *, const size_t);
extern bool gpsd_next_hunt_setting(struct gps_device_t *);
extern int gpsd_switch_driver(struct gps_device_t *, const char *);
extern void gpsd_set_speed(struct gps_device_t *, speed_t, char, unsigned int);
extern speed_t gpsd_get_speed(const struct gps_device_t *);
extern speed_t gpsd_get_speed_old(const struct gps_device_t *);
extern int gpsd_get_stopbits(const struct gps_device_t *);
extern char gpsd_get_parity(const struct gps_device_t *);
extern void gpsd_assert_sync(struct gps_device_t *);
extern void gpsd_close(struct gps_device_t *);

extern ssize_t gpsd_write(struct gps_device_t *, const uint8_t *, const size_t);
extern void gpsd_throttled_report(const int errlevel, const char * buf);

extern void gpsd_time_init(struct gps_context_t *, time_t);
extern void gpsd_set_century(struct gps_device_t *);
extern timestamp_t gpsd_gpstime_resolve(/*@in@ */ struct gps_device_t *,
			      const unsigned short, const double);
extern timestamp_t gpsd_utc_resolve(/*@in@*/struct gps_device_t *,
			      register struct tm *,
			      double);
extern void gpsd_century_update(/*@in@*/struct gps_device_t *, int);

extern void gpsd_zero_satellites(/*@out@*/struct gps_data_t *sp);
extern gps_mask_t gpsd_interpret_subframe(struct gps_device_t *, unsigned int,
				uint32_t[]);
extern gps_mask_t gpsd_interpret_subframe_raw(struct gps_device_t *,
				unsigned int, uint32_t[]);
extern /*@ observer @*/ const char *gpsd_hexdump(/*@out@*/char *, size_t,
						 /*@null@*/char *, size_t);
extern /*@ observer @*/ const char *gpsd_packetdump(/*@out@*/char *, size_t,
						    /*@null@*/char *, size_t);
extern /*@ observer @*/ const char *gpsd_prettydump(struct gps_device_t *);
# ifdef __cplusplus
extern "C" {
# endif
extern int gpsd_hexpack(/*@in@*/const char *, /*@out@*/uint8_t *, size_t);
# ifdef __cplusplus
}
# endif
extern ssize_t hex_escapes(/*@out@*/char *, const char *);
extern void gpsd_position_fix_dump(struct gps_device_t *,
				   /*@out@*/char[], size_t);
extern void gpsd_clear_data(struct gps_device_t *);
extern socket_t netlib_connectsock(int, const char *, const char *, const char *);
extern socket_t netlib_localsocket(const char *, int);
extern const char /*@observer@*/ *netlib_errstr(const int);
extern char /*@observer@*/ *netlib_sock2ip(socket_t);

extern void nmea_tpv_dump(struct gps_device_t *, /*@out@*/char[], size_t);
extern void nmea_sky_dump(struct gps_device_t *, /*@out@*/char[], size_t);
extern void nmea_subframe_dump(struct gps_device_t *, /*@out@*/char[], size_t);
extern void nmea_ais_dump(struct gps_device_t *, /*@out@*/char[], size_t);
extern void nmea_navigation_dump(struct gps_device_t *session,  /*@out@*/char[], size_t);
extern int nmea_environment_dump(struct gps_device_t *session, int num, /*@out@*/char[], size_t);
extern unsigned int ais_binary_encode(struct ais_t *ais, /*@out@*/unsigned char *bits, int flag);

extern gps_mask_t signalk_update_dump(struct gps_device_t *, 
                                      const struct vessel_t * vessel_t,
                                      /*@out@*/char[], size_t);

extern void ntpshm_context_init(struct gps_context_t *);
extern void ntpshm_session_init(struct gps_device_t *);
extern int ntpshm_put(struct gps_device_t *, int, struct timedrift_t *);
extern void ntpshm_latch(struct gps_device_t *device,  /*@out@*/struct timedrift_t *td);
extern void ntpshm_link_deactivate(struct gps_device_t *);
extern void ntpshm_link_activate(struct gps_device_t *);

/* normalize a timespec */
#define TS_NORM(ts)  \
    do { \
	if ( 1000000000 <= (ts)->tv_nsec ) { \
	    (ts)->tv_nsec -= 1000000000; \
	    (ts)->tv_sec++; \
	} else if ( 0 > (ts)->tv_nsec ) { \
	    (ts)->tv_nsec += 1000000000; \
	    (ts)->tv_sec--; \
	} \
    } while (0)

/* normalize a timeval */
#define TV_NORM(tv)  \
    do { \
	if ( 1000000 <= (tv)->tv_usec ) { \
	    (tv)->tv_usec -= 1000000; \
	    (tv)->tv_sec++; \
	} else if ( 0 > (tv)->tv_usec ) { \
	    (tv)->tv_usec += 1000000; \
	    (tv)->tv_sec--; \
	} \
    } while (0)

/* convert timespec to timeval, with rounding */
#define TSTOTV(tv, ts) \
    do { \
	(tv)->tv_sec = (ts)->tv_sec; \
	(tv)->tv_usec = ((ts)->tv_nsec + 500)/1000; \
        TV_NORM( tv ); \
    } while (0)

/* convert timeval to timespec */
#define TVTOTS(ts, tv) \
    do { \
	(ts)->tv_sec = (tv)->tv_sec; \
	(ts)->tv_nsec = (tv)->tv_usec*1000; \
        TS_NORM( ts ); \
    } while (0)

extern void pps_thread_activate(struct gps_device_t *);
extern void pps_thread_deactivate(struct gps_device_t *);
extern int pps_thread_lastpps(struct gps_device_t *, struct timedrift_t *);

extern void gpsd_acquire_reporting_lock(void);
extern void gpsd_release_reporting_lock(void);

extern void ecef_to_wgs84fix(/*@out@*/struct gps_fix_t *,
			     /*@out@*/double *,
			     double, double, double,
			     double, double, double);
extern void clear_dop(/*@out@*/struct dop_t *);

/* shmexport.c */
#define GPSD_KEY	0x47505344	/* "GPSD" */
struct shmexport_t
{
    int bookend1;
    struct gps_data_t gpsdata;
    int bookend2;
};
extern bool shm_acquire(struct gps_context_t *);
extern void shm_release(struct gps_context_t *);
extern void shm_update(struct gps_context_t *, struct gps_data_t *);


/* dbusexport.c */
#if defined(DBUS_EXPORT_ENABLE) && !defined(S_SPLINT_S)
int initialize_dbus_connection (void);
void send_dbus_fix (struct gps_device_t* channel);
#endif /* defined(DBUS_EXPORT_ENABLE) && !defined(S_SPLINT_S) */

/* srecord.c */
extern void hexdump(size_t, unsigned char *, unsigned char *);
extern unsigned char sr_sum(unsigned int, unsigned int, unsigned char *);
extern int bin2srec(unsigned int, unsigned int, unsigned int, unsigned char *, unsigned char *);
extern int srec_hdr(unsigned int, unsigned char *, unsigned char *);
extern int srec_fin(unsigned int, unsigned char *);
extern unsigned char hc(unsigned char);

/* application interface */
extern void gps_context_init(struct gps_context_t *context);
extern void gpsd_init(struct gps_device_t *,
		      struct gps_context_t *,
		      /*@null@*/const char *);
extern void gpsd_clear(struct gps_device_t *);
extern int gpsd_open(struct gps_device_t *);
#define O_CONTINUE	0
#define O_PROBEONLY	1
#define O_OPTIMIZE	2
extern int gpsd_activate(struct gps_device_t *, const int);
extern void gpsd_deactivate(struct gps_device_t *);

#define AWAIT_TIMEOUT	2
#define AWAIT_GOT_INPUT	1
#define AWAIT_NOT_READY	0
#define AWAIT_FAILED	-1
extern int gpsd_await_data(/*@out@*/fd_set *,
			    const int, 
			    /*@in@*/fd_set *,
			    const int);
extern gps_mask_t gpsd_poll(struct gps_device_t *);
#define DEVICE_EOF	-3
#define DEVICE_ERROR	-2
#define DEVICE_UNREADY	-1
#define DEVICE_READY	1
#define DEVICE_UNCHANGED	0
extern int gpsd_multipoll(const bool,
			  struct gps_device_t *,
			  void (*)(struct gps_device_t *, gps_mask_t),
			  float reawake_time);
extern void gpsd_wrap(struct gps_device_t *);
extern bool gpsd_add_device(const char *device_name, bool flag_nowait);
extern /*@observer@*/const char *gpsd_maskdump(gps_mask_t);

/* exceptional driver methods */
extern bool ubx_write(struct gps_device_t *, unsigned int, unsigned int,
		      /*@null@*/unsigned char *, size_t);
extern bool ais_binary_decode(const int debug,
			      struct ais_t *ais,
			      const unsigned char *, size_t,
			      /*@null@*/struct ais_type24_queue_t *);

/* debugging apparatus for the client library */
#ifdef CLIENTDEBUG_ENABLE
#define LIBGPS_DEBUG
#endif /* CLIENTDEBUG_ENABLE */
#ifdef LIBGPS_DEBUG
#define DEBUG_CALLS	1	/* shallowest debug level */
#define DEBUG_JSON	5	/* minimum level for verbose JSON debugging */
# define libgps_debug_trace(args) (void) libgps_trace args
extern int libgps_debuglevel;
extern void libgps_dump_state(struct gps_data_t *);
#else
# define libgps_debug_trace(args) /*@i1@*/do { } while (0)
#endif /* LIBGPS_DEBUG */

void gpsd_labeled_report(const int, const int, const int,
			 const char *, const char *, va_list);
# if __GNUC__ >= 3 || (__GNUC__ == 2 && __GNUC_MINOR__ >= 7)
__attribute__((__format__(__printf__, 3, 4))) void gpsd_report(const int, const int, const char *, ...);
__attribute__((__format__(__printf__, 3, 4))) void gpsd_external_report(const int, const int, const char *, ...);
# else /* not a new enough GCC, use the unprotected prototype */
void gpsd_report(const int, const int, const char *, ...);
void gpsd_external_report(const int, const int, const char *, ...);
#endif

int config_parse(struct interface_t *, struct vessel_t *, struct gps_device_t *);

#ifdef S_SPLINT_S
extern struct protoent *getprotobyname(const char *);
extern /*@observer@*/char *strptime(const char *,const char *tp,/*@out@*/struct tm *)/*@modifies tp@*/;
extern struct tm *gmtime_r(const time_t *,/*@out@*/struct tm *tp)/*@modifies tp@*/;
extern struct tm *localtime_r(const time_t *,/*@out@*/struct tm *tp)/*@modifies tp@*/;
#endif /* S_SPLINT_S */

/*
 * How to mix together epx and epy to get a horizontal circular error
 * eph when reporting requires it. Most devices don't report these;
 * NMEA 3.x devices reporting $GPGBS are the exception.
 */
#define EMIX(x, y)	(((x) > (y)) ? (x) : (y))

#define NITEMS(x) (int)(sizeof(x)/sizeof(x[0]))

/* Ugh - required for build on Solaris */
#ifndef NAN
#define NAN (0.0f/0.0f)
#endif

/* Cygwin, in addition to NAN, doesn't have cfmakeraw */
#if defined(__CYGWIN__)
void cfmakeraw(struct termios *);
#endif /* defined(__CYGWIN__) */

#define DEVICEHOOKPATH "/"SYSCONFDIR"/gpsd/device-hook"

/* Needed because 4.x versions of GCC are really annoying */
#define ignore_return(funcall)	assert(funcall != -23)

static /*@unused@*/ inline void memory_barrier(void)
{
#ifndef S_SPLINT_S
#if (__GNUC__ > 4) || (__GNUC__ == 4 && __GNUC_MINOR__ >= 7)
       __atomic_thread_fence(__ATOMIC_SEQ_CST);
#elif (__GNUC__ == 4 && __GNUC_MINOR__ >= 4)
       __sync_synchronize();
#endif
#endif /* S_SPLINT_S */
}

# ifdef __cplusplus
}
# endif

#endif /* _GPSD_H_ */
// Local variables:
// mode: c
// end:
