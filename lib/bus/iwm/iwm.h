#ifdef BUILD_APPLE
#ifndef IWM_H
#define IWM_H

#include "../../include/debug.h"

#include "bus.h"

#include <cstdint>
#include <forward_list>
#include <string>
#include "fnFS.h"

#undef TESTTX
//#define TESTTX

// these are for the temporary disk functions
//#include "fnFsSD.h"
//#include <string>

// class def'ns
class iwmFuji;     // declare here so can reference it, but define in fuji.h
class iwmModem;    // declare here so can reference it, but define in modem.h
class iwmNetwork;  // declare here so can reference it, but define in network.h
class iwmPrinter;  // Printer device
class iwmDisk;     // disk device cause I need to use "iwmDisk smort" for prototyping in iwmBus::service()

class iwmBus;      // forward declare bus so can be friend

// Sorry, this  is the protocol adapter's fault. -Thom
union cmdFrame_t
{
    struct
    {
        uint8_t device;
        uint8_t comnd;
        uint8_t aux1;
        uint8_t aux2;
        uint8_t cksum;
    };
    struct
    {
        uint32_t commanddata;
        uint8_t checksum;
    } __attribute__((packed));
};

enum class iwm_internal_type_t
{
  GenericBlock,
  GenericChar,
  FujiNet,
  Modem,
  Network,
  CPM,
  Printer,
  Voice
};

struct iwm_device_info_block_t
{
  std::string device_name; // limted to 16 chars std ascii (<128), no zero terminator
  uint8_t device_type;
  uint8_t device_subtype;
  uint8_t firmware_rev;
};

class iwmDevice
{
friend iwmBus; // put here for prototype, not sure if will need to keep it

protected:
  iwm_internal_type_t internal_type;
  iwm_device_info_block_t dib;
  uint8_t _devnum;
  bool _initialized;

  // device information block


  // iwm packet handling
  uint8_t packet_buffer[605]; //smartport packet buffer
  // todo: make a union with the first set of elements for command packet

  int decode_data_packet(void); //decode smartport 512 byte data packet

  void encode_data_packet(uint8_t source); //encode smartport 512 byte data packet
  void encode_write_status_packet(uint8_t source, uint8_t status);
  void encode_init_reply_packet(uint8_t source, uint8_t status);
  virtual void encode_status_reply_packet() = 0;
  void encode_error_reply_packet(uint8_t source);
  virtual void encode_status_dib_reply_packet() = 0;

  void encode_extended_data_packet(uint8_t source);
  virtual void encode_extended_status_reply_packet() = 0;
  virtual void encode_extended_status_dib_reply_packet() = 0;

  bool verify_cmdpkt_checksum(void);
  int packet_length(void);


#ifdef DEBUG
  void print_packet(uint8_t *data, int bytes);
  void print_packet();
#endif

  // void iwm_readblock();
 
  virtual void shutdown() = 0;
  virtual void process() = 0;

public:
  bool device_active;
  /**
   * @brief get the IWM device Number (1-255)
   * @return The device number registered for this device
   */
  int id() { return _devnum; };
  //void assign_id(uint8_t n) { _devnum = n; };

  /**
   * @brief Get the iwmBus object that this iwmDevice is attached to.
   */
  iwmBus iwm_get_bus();

  /**
   * Startup hack for now
   */
  virtual void startup_hack();
};

class iwmBus
{
private:
    std::forward_list<iwmDevice *> _daisyChain;


    iwmDevice *_activeDev = nullptr;
    
    iwmFuji *_fujiDev = nullptr;
    iwmModem *_modemDev = nullptr;
    iwmNetwork *_netDev[8] = {nullptr};
    //sioMIDIMaze *_midiDev = nullptr;
    //sioCassette *_cassetteDev = nullptr;
    //iwmCPM *_cpmDev = nullptr;
    iwmPrinter *_printerdev = nullptr;

    // low level bit-banging i/o functions
    struct iwm_timer_t
    {
        uint32_t tn;
        uint32_t t0;
    } iwm_timer;

    void timer_config();
    void iwm_timer_latch();
    void iwm_timer_read();
    void iwm_timer_alarm_set(int s);
    void iwm_timer_alarm_snooze(int s);
    void iwm_timer_wait();
    void iwm_timer_reset();

    void iwm_rddata_set();
    void iwm_rddata_clr();
    void iwm_rddata_disable();
    void iwm_rddata_enable();
    bool iwm_wrdata_val();
    bool iwm_req_val();
    void iwm_ack_set();
    void iwm_ack_clr();
    void iwm_ack_enable();
    void iwm_ack_disable();
    void iwm_extra_set();
    void iwm_extra_clr();

    bool iwm_phase_val(int p);
    
    enum class iwm_phases_t
    {
        idle = 0,
        reset,
        enable
    };
    iwm_phases_t iwm_phases();
#ifdef DEBUG
    iwm_phases_t oldphase;
#endif

public:
  int iwm_read_packet(uint8_t *a);
  int iwm_read_packet_timeout(int tout, uint8_t *a);
  int iwm_send_packet(uint8_t *a);

  void setup();
  void service();
  void shutdown();

  void handle_init(iwmDevice* smort); // todo: put this function in the right place

  int numDevices();
  void addDevice(iwmDevice *pDevice, iwm_internal_type_t deviceType); // todo: probably get called by handle_init()
  void remDevice(iwmDevice *pDevice);
  iwmDevice *deviceById(int device_id);
  void changeDeviceId(iwmDevice *p, int device_id);
  iwmDevice *smort;

#ifdef TESTTX
  void test_send(iwmDevice* smort);
#endif

};

extern iwmBus IWM;

#endif // guard
#endif /* BUILD_APPLE */