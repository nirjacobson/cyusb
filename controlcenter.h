#ifndef CONTROLCENTER_H
#define CONTROLCENTER_H

#include <QWidget>
#include <QSocketNotifier>
#include <QListWidgetItem>
#include <QFileDialog>
#include <QMessageBox>
#include <QElapsedTimer>
#include <QProgressBar>
#include <QStatusBar>
#include <QRegularExpression>

#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <signal.h>

#include "usbmethods.h"
#include "include/cyusb.h"

namespace Ui {
class ControlCenter;
}

class ControlCenter : public QWidget
{
    Q_OBJECT

public:
    explicit ControlCenter(QWidget *parent = nullptr);
    ~ControlCenter();

    static void unixhandler_sigusr1(int unused);
    QSocketNotifier *sn_sigusr1;

public slots:
    void on_pb_setIFace_clicked();
    void on_pb_setAltIf_clicked();
    void on_pb6_rcv_clicked();
    void pb6_send_nofile_selected();
    void pb6_send_file_selected(unsigned char *, int);
    void on_pb7_send_clicked();
    void appExit();
    void about();

private slots:
    void on_pb_kerneldetach_clicked();
    void on_listWidget_itemClicked(QListWidgetItem *item);

    void sigusr1_handler();

    void on_pb1_selfile_clicked();
    void on_pb1_start_clicked();
    void on_rb1_ram_clicked();
    void on_rb1_small_clicked();
    void on_rb1_large_clicked();
    void on_pb_reset_clicked();
    void on_rb3_ramdl_clicked();
    void on_rb3_ramup_clicked();
    void on_rb3_eedl_clicked();
    void on_rb3_eeup_clicked();
    void on_rb3_getchip_clicked();
    void on_rb3_renum_clicked();
    void on_rb3_custom_clicked();
    void on_rb3_out_clicked();
    void on_rb3_in_clicked();
    void on_pb3_selfile_clicked();
    void on_pb_execvc_clicked();
    void on_pb3_dl_clicked();
    void on_le3_out_hex_textEdited();
    void on_le3_out_ascii_textEdited();
    void on_le3_out_ascii_textChanged();
    void on_pb4_selfile_clicked();
    void on_pb4_start_clicked();
    void on_pb4_clear_clicked();
    void on_le6_out_hex_textEdited();
    void on_le6_out_ascii_textEdited();
    void on_le6_out_ascii_textChanged();
    void on_pb6_clear_clicked();
    void on_rb6_constant_clicked();
    void on_rb6_random_clicked();
    void on_rb6_inc_clicked();
    void on_pb6_send_clicked();
    void on_cb6_loop_clicked();
    void on_pb6_selout_clicked();
    void on_pb6_selin_clicked();
    void on_pb6_clearhalt_out_clicked();
    void on_pb6_clearhalt_in_clicked();
    void on_pb7_clear_clicked();
    void on_pb7_rcv_clicked();
    void on_rb7_enable_clicked();
    void on_rb7_disable_clicked();
    void on_pb7_clearhalt_out_clicked();
    void on_pb7_clearhalt_in_clicked();
    void on_streamer_control_start_clicked();
    void on_streamer_control_stop_clicked();

public:
    struct DEVICE_SUMMARY {
        int	ifnum;
        int	altnum;
        int	epnum;
        int	eptype;
        int	maxps;
        int	interval;
        int	reqsize;
    };

    static void setup_handler(int signo);
    void set_tool_tips();
    void update_devlist();
    QLabel* streamer_out_passcnt();
    QLabel* streamer_out_failcnt();
    QLabel* streamer_out_perf();
    QListWidget* lw1_display();


private:
    Ui::ControlCenter *ui;

    struct DEVICE_SUMMARY summ[100];
    int summ_count = 0;

    // Buffer used to assemble vendor command data to be transferred
    char le3_out_data[4096] = {0};
    // Buffer used to assemble bulk/iso data to be transferred
    char le6_out_data[4096] = {0};
    unsigned int cum_data_in;
    unsigned int cum_data_out;
    int fd_outfile, fd_infile;
    int data_count;
    struct libusb_transfer *transfer = NULL;
    unsigned char *isoc_databuf = NULL;
    int totalout, totalin, pkts_success, pkts_failure;
    QElapsedTimer *isoc_time;

    void check_for_kernel_driver();
    void update_endpoints();
    void clear_widgets();
    void dump_data(unsigned short num_bytes, char *dbuf);
    void update_summary();
    void get_config_details();
    void disable_vendor_extensions();
    void enable_vendor_extensions();
    void detect_device();
    void get_device_details();
    void set_if_aif();
    void clearhalt_in();
    void clearhalt_out();
    void dump_data6_in(int num_bytes, unsigned char *dbuf);
    void dump_data6_out(int num_bytes, unsigned char *dbuf);
    void dump_data7_in(int num_bytes, unsigned char *dbuf);
    void dump_data7_out(int num_bytes, unsigned char *dbuf);
    void in_callback( struct libusb_transfer *transfer);
    void out_callback( struct libusb_transfer *transfer);
};

#endif // CONTROLCENTER_H
