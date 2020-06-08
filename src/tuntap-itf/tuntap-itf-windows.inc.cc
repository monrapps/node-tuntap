#include "tuntap-itf.hh"

#include <cerrno>
#include <cstring>

//#include <endian.h>
//#include "portable_endian.hh"

//#define htobe32(x) htonl(x)
//#define be32toh(x) ntohl(x)

#include <uv.h>

/*
 * Static/local functions
 */
static void ifreqPrep(struct ifreq* ifr, const char* itf_name) {
	memset(ifr, 0, sizeof(*ifr));

	if (itf_name != NULL) {
		int len = strlen(itf_name);
		if (len > 0) {
			strncpy(
				ifr->ifr_name,
				itf_name,
				(IFNAMSIZ < len ? IFNAMSIZ : len)
			);
		}
	}
}

template <typename T>
static bool doIoctl(int fd, int opt, T data) {
	int ret;
	ret = ioctl(fd, opt, data);
	if (ret < 0)
		return(false);
	return(true);
}

template <typename T>
static bool doIfreq(int fd, struct ifreq* ifr, T* field, std::string addr, int port, int opt) {
	if (addr.size() > 0) {
		struct sockaddr_in sai;
		if (uv_ip4_addr(addr.c_str(), port, &sai) != 0)
			return(false);
		memcpy(field, &sai, sizeof(sai));
		return(doIoctl(fd, opt, ifr));
	}

	return(false);
}


/*
 * Public functions
 */
bool tuntapItfCreate(tuntap_itf_opts_t& opts, int* fd, std::string* err) {
#define RETURN(_e) { \
		if(err) \
			*err = std::string(_e) + " : " + strerror(errno); \
		return(false); \
	}

#define MK_IOCTL(fd, opt, data) {\
		if(doIoctl(fd, opt, data) == false) \
			RETURN("Error calling ioctl (" #opt ")") \
	}

#define MK_IFREQ_ADDR_IOCTL(fd, ifr_field, this_field, opt) \
	if(doIfreq(fd, &ifr, &ifr.ifr_field, opts.this_field, 0, opt) == false) \
		RETURN("Error calling ioctl (" #opt ")") \

	struct ifreq ifr;
	int tun_sock;

	/* First open the device */
	if ((*fd = ::open(TUNTAP_DFT_PATH, O_RDWR)) < 0)
		RETURN("Cannot open " TUNTAP_DFT_PATH)

		ifreqPrep(&ifr, opts.itf_name.c_str());

	if (opts.mode == tuntap_itf_opts_t::MODE_TUN)
		ifr.ifr_flags |= IFF_TUN;
	else if (opts.mode == tuntap_itf_opts_t::MODE_TAP)
		ifr.ifr_flags |= IFF_TAP;

	MK_IOCTL(*fd, TUNSETIFF, &ifr)
		if (strlen(ifr.ifr_name) > 0)
			opts.itf_name = ifr.ifr_name;

	MK_IOCTL(*fd, TUNSETPERSIST, opts.is_persistant ? 1 : 0)

		/* Then open a socket to change device parameters */
		tun_sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (tun_sock < 0)
		RETURN("Call of socket() failed!");

	ifr.ifr_mtu = opts.mtu;
	MK_IOCTL(tun_sock, SIOCSIFMTU, &ifr)

		if (opts.addr.size() > 0) {
			MK_IFREQ_ADDR_IOCTL(tun_sock, ifr_addr, addr, SIOCSIFADDR)
				MK_IFREQ_ADDR_IOCTL(tun_sock, ifr_netmask, mask, SIOCSIFNETMASK)
				MK_IFREQ_ADDR_IOCTL(tun_sock, ifr_dstaddr, dest, SIOCSIFDSTADDR)
		}

	ifr.ifr_flags |= (opts.is_up ? IFF_UP : 0) | (opts.is_running ? IFF_RUNNING : 0);
	MK_IOCTL(tun_sock, SIOCSIFFLAGS, &ifr)

		::close(tun_sock);

#undef RETURN
#undef MK_IOCTL
#undef MK_IFREQ_ADDR_IOCTL

	return(true);
}

bool tuntapItfSet(const std::vector<tuntap_itf_opts_t::option_e>& options, const tuntap_itf_opts_t& data, std::string* err) {
	struct ifreq ifr;
	int fd;

	fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (fd < 0) {
		if (err)
			*err = "Call of socket() failed!";
		return(false);
	}

	ifreqPrep(&ifr, data.itf_name.c_str());

	for (unsigned i = 0; i < options.size(); i++) {
		switch (options[i]) {
		case tuntap_itf_opts_t::OPT_ADDR:
			if (data.addr.length() == 0)
				doIfreq(fd, &ifr, &ifr.ifr_addr, "0.0.0.0", 0, SIOCSIFADDR);
			else
				doIfreq(fd, &ifr, &ifr.ifr_addr, data.addr.c_str(), 0, SIOCSIFADDR);
			break;
		case tuntap_itf_opts_t::OPT_MASK:
			doIfreq(fd, &ifr, &ifr.ifr_netmask, data.mask.c_str(), 0, SIOCSIFNETMASK);
			break;
		case tuntap_itf_opts_t::OPT_DEST:
			doIfreq(fd, &ifr, &ifr.ifr_dstaddr, data.dest.c_str(), 0, SIOCSIFDSTADDR);
			break;
		case tuntap_itf_opts_t::OPT_MTU:
			doIoctl(fd, SIOCSIFMTU, data.mtu);
			break;
		case tuntap_itf_opts_t::OPT_PERSIST:
			doIoctl(fd, TUNSETPERSIST, data.is_persistant ? 1 : 0);
			break;
		case tuntap_itf_opts_t::OPT_UP:
			doIoctl(fd, SIOCGIFFLAGS, &ifr);
			if (data.is_up)
				ifr.ifr_flags |= IFF_UP;
			else
				ifr.ifr_flags &= ~(IFF_UP);
			doIoctl(fd, SIOCSIFFLAGS, &ifr);
			break;
		case tuntap_itf_opts_t::OPT_RUNNING:
			doIoctl(fd, SIOCGIFFLAGS, &ifr);
			if (data.is_running)
				ifr.ifr_flags |= IFF_RUNNING;
			else
				ifr.ifr_flags &= ~(IFF_RUNNING);
			doIoctl(fd, SIOCSIFFLAGS, &ifr);
			break;
		}
	}

	::close(fd);

	return(true);
}