#include "controlcenter.h"
#include "ui_controlcenter.h"

int sigusr1_fd[2];

cyusb_handle  *h = NULL;
int num_devices_detected;
int current_device_index = -1;

ControlCenter::ControlCenter(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::ControlCenter)
{
    ui->setupUi(this);

    if ( socketpair(AF_UNIX, SOCK_STREAM, 0, sigusr1_fd) )
        qFatal("Couldn't create SIGUSR1 socketpair");

    sn_sigusr1 = new QSocketNotifier(sigusr1_fd[1], QSocketNotifier::Read, this);
    connect(sn_sigusr1, SIGNAL(activated(int)), this, SLOT(sigusr1_handler()));

    QStringList list;
    list.clear();
    list << "1" << "2" << "4" << "8" << "16" << "32" << "64" << "128";
    ui->cb7_numpkts->addItems(list);
    ui->rb7_enable->setChecked(false); /* disabled by default for maximum performance */
    ui->rb7_disable->setChecked(true);

    int r = cyusb_open();
    if ( r < 0 ) {
        printf("Error opening library\n");
    }
    else if ( r == 0 ) {
        printf("No device found\n");
    }
    else num_devices_detected = r;

    signal(SIGUSR1, ControlCenter::setup_handler);
}

ControlCenter::~ControlCenter()
{
    delete ui;
}

void ControlCenter::on_pb4_start_clicked()
{
    int r = 0;

    if ( ui->rb4_ram->isChecked() )
        r = fx3_usbboot_download(qPrintable(ui->label4_file->text()));
    else if ( ui->rb4_i2c->isChecked() )
        r = fx3_i2cboot_download(qPrintable(ui->label4_file->text()));
    else if ( ui->rb4_spi->isChecked() )
        r = fx3_spiboot_download(qPrintable(ui->label4_file->text()));

    if ( r ) {
        QMessageBox mb;
        mb.setText("Error in download");
        mb.exec();
    }
    else {
        QMessageBox mb;
        mb.setText("Successfully downloaded");
        mb.exec();
    }
}

void ControlCenter::on_pb4_clear_clicked()
{
    ui->lw4_display->clear();
}

void ControlCenter::on_streamer_control_start_clicked ()
{
    const char  *temp;
    unsigned int ep;
    unsigned int eptype;
    unsigned int pktsize;
    unsigned int reqsize;
    unsigned int queuedepth;

    bool ok;
    int  iface   = ui->label_if->text().toInt(&ok, 10);
    int  aiface  = ui->label_aif->text().toInt(&ok, 10);

    // Disable the start button
    ui->streamer_control_start->setEnabled (false);

    // Get the streamer parameters by querying the combo boxes.
    temp = ui->streamer_ep_sel->currentText().toStdString().c_str();
    sscanf (temp + 3, "%d", &ep);
    if ((temp[6] == 'I') && (temp[7] == 'N'))
        ep |= 0x80;

    if (memcmp ((void *)&temp[11], "BULK", 4) == 0) {
        eptype = LIBUSB_TRANSFER_TYPE_BULK;
    } else {
        if (memcmp ((void *)&temp[11], "ISOC", 4) == 0) {
            eptype = LIBUSB_TRANSFER_TYPE_ISOCHRONOUS;

            // Select the alternate interface again, so that the
            // host frees up enough bandwidth for the endpoint.
            cyusb_set_interface_alt_setting (h, iface, aiface);
        }
        else {
            eptype = LIBUSB_TRANSFER_TYPE_INTERRUPT;
        }
    }

    sscanf (temp + 16, "%d", &pktsize);

    temp = ui->streamer_size_sel->currentText().toStdString().c_str();
    sscanf (temp, "%d", &reqsize);
    temp = ui->streamer_queue_sel->currentText().toStdString().c_str();
    sscanf (temp, "%d", &queuedepth);

    streamer_set_params (ep, eptype, pktsize, reqsize, queuedepth);
    if (streamer_start_xfer () == 0) {
        // Test started properly. Enable the stop button.
        ui->streamer_control_stop->setEnabled (true);
    } else {
        // Test could not start. Re-enable the start button.
        ui->streamer_control_start->setEnabled (false);
    }
}

void ControlCenter::on_streamer_control_stop_clicked ()
{
    // Wait until streamer operation is stopped.
    streamer_stop_xfer ();
    while (streamer_is_running ())
        sleep (1);

    // Now disable the stop button and enable the start button.
    ui->streamer_control_stop->setEnabled (false);
    ui->streamer_control_start->setEnabled (true);

    ui->streamer_out_passcnt->setText ("0");
    ui->streamer_out_failcnt->setText ("0");
    ui->streamer_out_perf->setText ("0");
}

static void libusb_error(int errnum, const char *detailedText)
{
    char msg[30];
    char tbuf[60];

    memset(msg,'\0',30);
    memset(tbuf,'\0',60);
    QMessageBox mb;
    if ( errnum == LIBUSB_ERROR_IO )
        strcpy(msg, "LIBUSB_ERROR_IO");
    else if ( errnum == LIBUSB_ERROR_INVALID_PARAM )
        strcpy(msg, "LIBUSB_ERROR_INVALID_PARAM" );
    else if ( errnum == LIBUSB_ERROR_ACCESS )
        strcpy(msg, "LIBUSB_ERROR_ACCESS");
    else if ( errnum == LIBUSB_ERROR_NO_DEVICE )
        strcpy(msg, "LIBUSB_ERROR_NO_DEVICE");
    else if ( errnum == LIBUSB_ERROR_NOT_FOUND )
        strcpy(msg, "LIBUSB_ERROR_NOT_FOUND");
    else if ( errnum == LIBUSB_ERROR_BUSY )
        strcpy(msg, "LIBUSB_ERROR_BUSY");
    else if ( errnum == LIBUSB_ERROR_TIMEOUT )
        strcpy(msg, "LIBUSB_ERROR_TIMEOUT");
    else if ( errnum == LIBUSB_ERROR_OVERFLOW )
        strcpy(msg, "LIBUSB_ERROR_OVERFLOW");
    else if ( errnum == LIBUSB_ERROR_PIPE )
        strcpy(msg, "LIBUSB_ERROR_PIPE");
    else if ( errnum == LIBUSB_ERROR_INTERRUPTED )
        strcpy(msg, "LIBUSB_ERROR_INTERRUPTED");
    else if ( errnum == LIBUSB_ERROR_NO_MEM )
        strcpy(msg, "LIBUSB_ERROR_NO_MEM");
    else if ( errnum == LIBUSB_ERROR_NOT_SUPPORTED )
        strcpy(msg, "LIBUSB_ERROR_NOT_SUPPORTED");
    else if ( errnum == LIBUSB_ERROR_OTHER )
        strcpy(msg, "LIBUSB_ERROR_OTHER");
    else strcpy(msg, "LIBUSB_ERROR_UNDOCUMENTED");

    sprintf(tbuf,"LIBUSB_ERROR NO : %d, %s",errnum,msg);
    mb.setText(tbuf);
    mb.setDetailedText(detailedText);
    mb.exec();
    return;
}

void ControlCenter::on_pb_setIFace_clicked()
{
    int r;
    char tval[3];
    int N, M;

    struct libusb_config_descriptor *config_desc = NULL;

    r = cyusb_get_active_config_descriptor(h, &config_desc);
    if ( r ) libusb_error(r, "Error in 'get_active_config_descriptor' ");

    N = config_desc->interface[ui->sb_selectIf->value()].num_altsetting;
    sprintf(tval,"%d",N);
    ui->le_numAlt->setText(tval);
    ui->sb_selectAIf->setMaximum(N - 1);
    ui->sb_selectAIf->setValue(0);
    ui->sb_selectAIf->setEnabled(true);
    ui->pb_setAltIf->setEnabled(true);
    M = ui->sb_selectIf->value();
    sprintf(tval,"%d",M);
    ui->label_if->setText(tval);
    on_pb_setAltIf_clicked();
}

void ControlCenter::check_for_kernel_driver(void)
{
    int r;
    int v = ui->sb_selectIf->value();

    r = cyusb_kernel_driver_active(h, v);
    if ( r == 1 ) {
        ui->cb_kerneldriver->setEnabled(true);
        ui->cb_kerneldriver->setChecked(true);
        ui->pb_kerneldetach->setEnabled(true);
        ui->cb_kerneldriver->setEnabled(false);
    }
    else {
        ui->cb_kerneldriver->setEnabled(false);
        ui->cb_kerneldriver->setChecked(false);
        ui->pb_kerneldetach->setEnabled(false);
    }
}

void ControlCenter::update_endpoints()
{
    bool ok;
    char tbuf[32];
    int i;
    bool epfound = false;

    int iface   = ui->label_if->text().toInt(&ok, 10);
    int aiface  = ui->label_aif->text().toInt(&ok, 10);

    // Clear bulk endpoint list
    ui->cb6_in->clear();
    ui->cb6_out->clear();

    // Clear iso endpoint list
    ui->cb7_in->clear();
    ui->cb7_out->clear();

    // Clear streaming endpoint list and widgets
    ui->streamer_ep_sel->clear ();
    ui->streamer_size_sel->clear ();
    ui->streamer_queue_sel->clear ();
    ui->streamer_out_passcnt->setText ("0");
    ui->streamer_out_failcnt->setText ("0");
    ui->streamer_out_perf->setText ("0");
    ui->streamer_control_start->setEnabled (false);
    ui->streamer_control_stop->setEnabled (false);

    for ( i = 0; i < summ_count; ++i ) {
        if ( summ[i].ifnum != iface ) continue;
        if ( summ[i].altnum != aiface ) continue;

        // Add endpoints to the BULK and ISOCHRONOUS transfer tabs
        switch (summ[i].eptype)
        {
        case LIBUSB_TRANSFER_TYPE_BULK:
            sprintf(tbuf,"%02x",summ[i].epnum);
            if ( summ[i].epnum & 0x80 )
                ui->cb6_in->addItem(tbuf);
            else
                ui->cb6_out->addItem(tbuf);
            break;

        case LIBUSB_TRANSFER_TYPE_ISOCHRONOUS:
            sprintf(tbuf,"%02x",summ[i].epnum);
            if ( summ[i].epnum & 0x80 ) {
                ui->cb7_in->addItem(tbuf);
            }
            else {
                ui->cb7_out->addItem(tbuf);
            }
            break;

        default:
            break;
        }

        // Add endpoints to the streamer tab
        if (summ[i].epnum & 0x80) {
            switch (summ[i].eptype) {
            case LIBUSB_TRANSFER_TYPE_BULK:
                sprintf (tbuf, "EP %2d-IN:  BULK: %d BYTE",
                        summ[i].epnum & 0x0F, summ[i].reqsize);
                break;
            case LIBUSB_TRANSFER_TYPE_INTERRUPT:
                sprintf (tbuf, "EP %2d-IN:  INTR: %d BYTE",
                        summ[i].epnum & 0x0F, summ[i].reqsize);
                break;
            case LIBUSB_TRANSFER_TYPE_ISOCHRONOUS:
                sprintf (tbuf, "EP %2d-IN:  ISOC: %d BYTE",
                        summ[i].epnum & 0x0F, summ[i].reqsize);
                break;
            default:
                break;
            }
        } else {
            switch (summ[i].eptype) {
            case LIBUSB_TRANSFER_TYPE_BULK:
                sprintf (tbuf, "EP %2d-OUT: BULK: %d BYTE",
                        summ[i].epnum & 0x0F, summ[i].reqsize);
                break;
            case LIBUSB_TRANSFER_TYPE_INTERRUPT:
                sprintf (tbuf, "EP %2d-OUT: INTR: %d BYTE",
                        summ[i].epnum & 0x0F, summ[i].reqsize);
                break;
            case LIBUSB_TRANSFER_TYPE_ISOCHRONOUS:
                sprintf (tbuf, "EP %2d-OUT: ISOC: %d BYTE",
                        summ[i].epnum & 0x0F, summ[i].reqsize);
                break;
            default:
                break;
            }
        }

        ui->streamer_ep_sel->addItem(tbuf);
        epfound = true;
    }

    if (epfound) {
        // Add a range of values to the request size and queue depth lists
        for (int size = 1; size <= 512; size *= 2) {
            sprintf (tbuf, "%d", size);
            ui->streamer_size_sel->addItem (tbuf);
            ui->streamer_queue_sel->addItem (tbuf);
        }

        // Choose 16 as the default request size and queue depth
        ui->streamer_size_sel->setCurrentIndex (4);
        ui->streamer_queue_sel->setCurrentIndex (4);

        ui->streamer_control_start->setEnabled (true);
        ui->streamer_control_stop->setEnabled (false);
    }
}

void ControlCenter::on_pb_setAltIf_clicked()
{
    int r1, r2;
    char tval[3];
    int i = ui->sb_selectIf->value();
    int a = ui->sb_selectAIf->value();

    r1 = cyusb_claim_interface(h, i);
    if ( r1 == 0 ) {
        r2 = cyusb_set_interface_alt_setting(h, i, a);
        if ( r2 != 0 ) {
            libusb_error(r2, "Error in setting Alternate Interface");
        }
    }
    else {
        libusb_error(r1, "Error in claiming interface");
    }
    sprintf(tval,"%d",a);
    ui->label_aif->setText(tval);
    check_for_kernel_driver();
    update_endpoints();
}

void ControlCenter::clear_widgets()
{
    ui->lw_desc->clear();
    ui->label_if->clear();
    ui->label_aif->clear();
    ui->le_numIfaces->clear();
    ui->le_numAlt->clear();
    ui->sb_selectIf->clear();
    ui->sb_selectIf->setEnabled(false);
    ui->sb_selectAIf->setValue(0);
    ui->sb_selectAIf->setEnabled(false);
    ui->lw_desc->clear();
    ui->cb_kerneldriver->setChecked(false);
    ui->cb_kerneldriver->setEnabled(false);
    ui->pb_setIFace->setEnabled(false);
    ui->pb_kerneldetach->setEnabled(false);
    ui->pb_setAltIf->setEnabled(false);
    ui->label_devtype->setText("");

    // Clear widgets on streamer tab
    ui->streamer_ep_sel->clear ();
    ui->streamer_size_sel->clear ();
    ui->streamer_queue_sel->clear ();
    ui->streamer_out_passcnt->setText ("0");
    ui->streamer_out_failcnt->setText ("0");
    ui->streamer_out_perf->setText ("0");
    ui->streamer_control_start->setEnabled (false);
    ui->streamer_control_stop->setEnabled (false);
}

void ControlCenter::on_pb_kerneldetach_clicked()
{
    int r;

    r = cyusb_detach_kernel_driver(h, ui->sb_selectIf->value());
    if ( r == 0 ) {
        ui->cb_kerneldriver->setEnabled(true);
        ui->cb_kerneldriver->setChecked(false);
        ui->cb_kerneldriver->setEnabled(false);
        ui->pb_kerneldetach->setEnabled(false);
        ui->label_aif->clear();
    }
    else {
        libusb_error(r, "Error in detaching kernel mode driver!");
        return;
    }
}

void ControlCenter::on_rb1_ram_clicked()
{
    ui->groupBox_3->setVisible(true);
    ui->groupBox_3->setEnabled(true);
    ui->rb_internal->setChecked(true);
}

void ControlCenter::on_rb1_small_clicked()
{
    ui->groupBox_3->setVisible(false);
}

void ControlCenter::on_rb1_large_clicked()
{
    ui->groupBox_3->setVisible(false);
}

void ControlCenter::on_pb1_start_clicked()
{
    int r;

    if ( ui->rb1_ram->isChecked() ) {
        if ( ui->rb_internal->isChecked() )
            r = fx2_ram_download(qPrintable(ui->label1_selfile->text()), 0);
        else
            r = fx2_ram_download(qPrintable(ui->label1_selfile->text()), 1);
    }
    else {
        if ( ui->rb1_large->isChecked() )
            r = fx2_eeprom_download(qPrintable(ui->label1_selfile->text()), 1);
        else
            r = fx2_eeprom_download(qPrintable(ui->label1_selfile->text()), 0);
    }

    if ( r ) {
        QMessageBox mb;
        mb.setText("Error in download");
        mb.exec();
    }
    else {
        QMessageBox mb;
        mb.setText("Successfully downloaded");
        mb.exec();
    }
}

void ControlCenter::on_rb3_ramdl_clicked()
{
    ui->rb3_out->setChecked(true);
    ui->gb_dir->setEnabled(false);
    ui->le3_bm->setText("40");
    ui->le3_bm->setReadOnly(true);
    ui->le3_br->setText("A3");
    ui->le3_br->setReadOnly(true);
    ui->le3_wlen->setReadOnly(true);
    ui->le3_out_hex->setEnabled(true);
    ui->le3_out_ascii->setEnabled(true);
    ui->le3_wind->setText("0000");
    ui->le3_wind->setReadOnly(true);
    ui->le3_wlen->setText("");
}

void ControlCenter::on_rb3_ramup_clicked()
{
    ui->rb3_in->setChecked(true);
    ui->gb_dir->setEnabled(false);
    ui->le3_bm->setText("C0");
    ui->le3_bm->setReadOnly(true);
    ui->le3_br->setText("A3");
    ui->le3_br->setReadOnly(true);
    ui->le3_wlen->setReadOnly(false);
    ui->le3_out_hex->setEnabled(false);
    ui->le3_out_ascii->setEnabled(false);
    ui->le3_wind->setText("0000");
    ui->le3_wind->setReadOnly(true);

}

void ControlCenter::on_rb3_eedl_clicked()
{
    ui->rb3_out->setChecked(true);
    ui->gb_dir->setEnabled(false);
    ui->le3_bm->setText("40");
    ui->le3_bm->setReadOnly(true);
    ui->le3_br->setText("A2");
    ui->le3_br->setReadOnly(true);
    ui->le3_wlen->setReadOnly(true);
    ui->le3_out_hex->setEnabled(true);
    ui->le3_out_ascii->setEnabled(true);
    ui->le3_wind->setText("0000");
    ui->le3_wind->setReadOnly(true);
    ui->le3_wlen->setText("");
}


void ControlCenter::on_rb3_eeup_clicked()
{
    ui->rb3_in->setChecked(true);
    ui->gb_dir->setEnabled(false);
    ui->le3_bm->setText("C0");
    ui->le3_bm->setReadOnly(true);
    ui->le3_br->setText("A2");
    ui->le3_br->setReadOnly(true);
    ui->le3_wlen->setReadOnly(false);
    ui->le3_out_hex->setEnabled(false);
    ui->le3_out_ascii->setEnabled(false);
    ui->le3_wind->setText("0000");
    ui->le3_wind->setReadOnly(true);
}

void ControlCenter::on_rb3_getchip_clicked()
{
    ui->gb_dir->setEnabled(false);
    ui->le3_bm->setText("C0");
    ui->le3_bm->setReadOnly(true);
    ui->le3_br->setText("A6");
    ui->le3_br->setReadOnly(true);
    ui->le3_out_hex->setEnabled(false);
    ui->le3_out_ascii->setEnabled(false);
    ui->le3_wind->setText("0000");
    ui->le3_wind->setReadOnly(true);
    ui->le3_wlen->setText("01");
    ui->le3_wlen->setReadOnly(true);
}

void ControlCenter::on_rb3_renum_clicked()
{
    ui->gb_dir->setEnabled(false);
    ui->le3_bm->setText("40");
    ui->le3_bm->setReadOnly(true);
    ui->le3_br->setText("A8");
    ui->le3_br->setReadOnly(true);
    ui->le3_out_hex->setEnabled(false);
    ui->le3_out_ascii->setEnabled(false);
    ui->le3_wind->setText("0000");
    ui->le3_wind->setReadOnly(true);
    ui->le3_wlen->setText("01");
    ui->le3_wlen->setReadOnly(true);
}

void ControlCenter::on_rb3_custom_clicked()
{
    ui->rb3_out->setChecked(true);
    ui->gb_dir->setEnabled(true);
    ui->le3_bm->setReadOnly(false);
    ui->le3_bm->setText("");
    ui->le3_br->setText("");
    ui->le3_br->setReadOnly(false);
    ui->le3_wlen->setReadOnly(false);
    ui->le3_wind->setText("");
    ui->le3_wind->setReadOnly(false);
    ui->le3_out_hex->setEnabled(true);
    ui->le3_out_ascii->setEnabled(true);
    ui->le3_wlen->setText("");
}


void ControlCenter::on_rb3_out_clicked()
{
    if ( !ui->rb3_custom->isChecked() )
        ui->le3_bm->setText("40");
    ui->le3_out_hex->setEnabled(true);
    ui->le3_out_ascii->setEnabled(true);
    ui->le3_wlen->setReadOnly(true);
}

void ControlCenter::on_rb3_in_clicked()
{
    if ( !ui->rb3_custom->isChecked() )
        ui->le3_bm->setText("C0");
    ui->le3_out_hex->setEnabled(false);
    ui->le3_out_ascii->setEnabled(false);
    ui->le3_wlen->setReadOnly(false);
}

void ControlCenter::on_le3_out_ascii_textChanged()
{
    char tbuf[5];
    unsigned int sz;

    QByteArray t = ui->le3_out_ascii->text().toUtf8();

    sz = (t.size() > 4096) ? 4096 : t.size();
    memcpy (le3_out_data, (const char *)t.data(), sz);

    sprintf(tbuf,"%04d", sz);
    ui->le3_wlen->setText(tbuf);
}

void ControlCenter::on_le3_out_ascii_textEdited()
{
    char tbuf[5];
    unsigned int sz;

    QByteArray t = ui->le3_out_ascii->text().toUtf8();

    sz = (t.size() > 4096) ? 4096 : t.size();
    memcpy (le3_out_data, (const char *)t.data(), sz);

    sprintf(tbuf,"%04d", sz);
    ui->le3_wlen->setText(tbuf);
}

void ControlCenter::on_le3_out_hex_textEdited()
{
    char tbuf[5];
    unsigned int sz;

    QRegExp rx("[0-9A-Fa-f]*");
    QValidator *validator = new QRegExpValidator(rx, this);
    ui->le3_out_hex->setValidator(validator);

    QByteArray t = ui->le3_out_hex->text().toUtf8();
    QByteArray t2 = QByteArray::fromHex(t);

    sz = (t2.size() > 4096) ? 4096 : t2.size();
    memcpy (le3_out_data, (const char *)t2.data(), sz);

    sprintf (tbuf,"%04d", sz);
    ui->le3_wlen->setText(tbuf);
}

void ControlCenter::dump_data(unsigned short num_bytes, char *dbuf)
{
    int i, j, k, index, filler;
    char ttbuf[10];
    char finalbuf[256];
    char tbuf[256];
    int balance = 0;

    balance = num_bytes;
    index = 0;

    while ( balance > 0 ) {
        tbuf[0]  = '\0';
        if ( balance < 16 )
            j = balance;
        else j = 16;
        for ( i = 0; i < j; ++i ) {
            sprintf(ttbuf,"%02x ",(unsigned char)dbuf[index+i]);
            strcat(tbuf,ttbuf);
        }
        if ( balance < 16 ) {
            filler = 16-balance;
            for ( k = 0; k < filler; ++k )
                strcat(tbuf,"   ");
        }
        strcat(tbuf,": ");
        for ( i = 0; i < j; ++i ) {
            if ( !isprint(dbuf[index+i]) )
                strcat(tbuf,". ");
            else {
                sprintf(ttbuf,"%c ",dbuf[index+i]);
                strcat(tbuf,ttbuf);
            }
        }
        sprintf(finalbuf,"%s",tbuf);
        ui->lw3->addItem(finalbuf);
        balance -= j;
        index += j;
    }
}

void ControlCenter::on_pb_execvc_clicked()
{
    int r;

    ui->lw3->clear();

    if ( ( ui->le3_bm->text() == "" )
        ||   ( ui->le3_br->text() == "" )
        ||   ( ui->le3_wval->text() == "" )
        ||   (ui->le3_wind->text() == "" )
        ||   (ui->le3_wlen->text() == "" ) ) {
        QMessageBox mb;
        mb.setText("Please fill up ALL fields\n");
        mb.exec();
        return ;
    }

    unsigned char bmReqType = strtoul(qPrintable(ui->le3_bm->text()), NULL, 16);
    unsigned char bRequest  = strtoul(qPrintable(ui->le3_br->text()), NULL, 16);
    unsigned short wValue   = strtoul(qPrintable(ui->le3_wval->text()), NULL, 16);
    unsigned short wIndex   = strtoul(qPrintable(ui->le3_wind->text()), NULL, 16);
    unsigned short wLength  = strtoul(qPrintable(ui->le3_wlen->text()),NULL, 10);
    char data[4096];
    char msg[64];


    printf("bmReqType=%02x, bRequest=%02x, wValue=%04x, wIndex=%04x, wLength=%d\n",
           bmReqType, bRequest, wValue, wIndex, wLength);

    if ( bmReqType == 0x40 ) {
        r = cyusb_control_transfer(h, bmReqType, bRequest, wValue, wIndex, (unsigned char *)le3_out_data, wLength, 1000);
        if (r != wLength ) {
            sprintf (msg, "Vendor command failed\n\n");
            ui->lw3->addItem(msg);
        }
        else {
            if (wLength != 0) {
                dump_data (wLength, le3_out_data);
                sprintf (msg, "Vendor command succeeded\n\n");
                ui->lw3->addItem (msg);
            } else {
                sprintf (msg, "Vendor command with no data succeeded\n\n");
                ui->lw3->addItem (msg);
            }
        }
    }
    else {
        memset(data,' ',4096);
        r = cyusb_control_transfer(h, bmReqType, bRequest, wValue, wIndex, (unsigned char *)data, wLength, 1000);
        if (r != wLength ) {
            sprintf (msg, "Vendor command failed\n\n");
            ui->lw3->addItem(msg);
        }
        else {
            if (r != 0) {
                dump_data(r, data);
                sprintf (msg, "Vendor command succeeded\n\n");
                ui->lw3->addItem (msg);
            } else {
                sprintf (msg, "Vendor command with no data succeeded\n\n");
                ui->lw3->addItem (msg);
            }
        }
    }
}


void ControlCenter::on_pb3_dl_clicked()
{
    fx2_ram_download(qPrintable(ui->lab3_selfile->text()), 0);
    QMessageBox mb;
    mb.setText("Completed download");
    mb.exec();
    return ;
}



void ControlCenter::on_le6_out_hex_textEdited()
{
    char tbuf[5];
    unsigned int sz;

    QRegExp rx("[0-9A-Fa-f]*");
    QValidator *validator = new QRegExpValidator(rx, this);
    ui->le6_out_hex->setValidator(validator);

    QByteArray t = ui->le6_out_hex->text().toUtf8();
    QByteArray t2 = QByteArray::fromHex(t);

    sz = (t2.size () > 4096) ? 4096 : t2.size();
    memcpy (le6_out_data, (const char *)t2.data(), sz);

    ui->le6_out_ascii->setText("");
    ui->le6_outfile->setText("");

    sprintf(tbuf,"%04d", sz);
    ui->le6_size->setText(tbuf);
}

void ControlCenter::update_devlist()
{
    int i, r, num_interfaces, index = 0;
    char tbuf[60];
    struct libusb_config_descriptor *config_desc = NULL;

    ui->listWidget->clear();

    for ( i = 0; i < num_devices_detected; ++i ) {
        h = cyusb_gethandle(i);
        sprintf(tbuf,"VID=%04x,PID=%04x,BusNum=%02x,Addr=%d",
                cyusb_getvendor(h), cyusb_getproduct(h),
                cyusb_get_busnumber(h), cyusb_get_devaddr(h));
        ui->listWidget->addItem(QString(tbuf));
        r = cyusb_get_active_config_descriptor (h, &config_desc);
        if ( r ) {
            libusb_error(r, "Error in 'get_active_config_descriptor' ");
            return;
        }
        num_interfaces = config_desc->bNumInterfaces;
        while (num_interfaces){
            if (cyusb_kernel_driver_active (h, index)){
                cyusb_detach_kernel_driver (h, index);
            }
            index++;
            num_interfaces--;
        }
        cyusb_free_config_descriptor (config_desc);
    }
}

QLabel *ControlCenter::streamer_out_passcnt()
{
    return ui->streamer_out_passcnt;
}

QLabel *ControlCenter::streamer_out_failcnt()
{
    return ui->streamer_out_failcnt;
}

QLabel *ControlCenter::streamer_out_perf()
{
    return ui->streamer_out_perf;
}

QListWidget *ControlCenter::lw1_display()
{
    return ui->lw1_display;
}

void ControlCenter::on_pb4_selfile_clicked()
{
    QString filename;
    if ( (current_device_index == -1) || (ui->label_if->text() == "") || (ui->label_aif->text() == "") ) {
        QMessageBox mb;
        mb.setText("No device+iface+alt-iface has been selected yet !\n");
        mb.exec();
        return ;
    }
    filename = QFileDialog::getOpenFileName(this, "Select file to download...", ".", "Image files (*.img)");
    ui->label4_file->setText(filename);
    if ( filename != "" ) {
        ui->pb4_start->setEnabled(true);
    }

}
void ControlCenter::update_summary(void)
{
    char tbuf[100];
    int i;
    char ifnum[7];
    char altnum[7];
    char epnum[7];
    char iodirn[7];
    char iotype[7];
    char maxps[7];
    char interval[7];

    ifnum[6] = altnum[6] = epnum[6] = iodirn[6] = iotype[6] = maxps[6] = interval[6] = '\0';

    memset(tbuf,'\0',100);
    ui->lw_summ->clear();

    for ( i = 0; i < summ_count; ++i ) {
        sprintf(ifnum,"%2d    ",summ[i].ifnum);
        sprintf(altnum,"%2d    ",summ[i].altnum);
        sprintf(epnum,"%2x    ",summ[i].epnum);
        if ( summ[i].epnum & 0x80 )
            strcpy(iodirn,"IN ");
        else strcpy(iodirn,"OUT");
        if ( summ[i].eptype & 0x00 )
            strcpy(iotype,"CTRL");
        else if ( summ[i].eptype & 0x01 )
            strcpy(iotype,"Isoc");
        else if ( summ[i].eptype & 0x02 )
            strcpy(iotype,"Bulk");
        else strcpy(iotype,"Intr");
        sprintf(maxps,"%4d  ",summ[i].maxps);
        sprintf(interval,"%3d   ",summ[i].interval);
        if ( i == 0 ) {
            sprintf(tbuf,"%-6s %-6s %-6s %-6s %-6s %-6s %-6s",
                    ifnum,altnum,epnum,iodirn,iotype,maxps,interval);
            ui->lw_summ->addItem(QString(tbuf));
        }
        else {
            if ( summ[i].ifnum == summ[i-i].ifnum ) {
                memset(ifnum,' ',6);
                if ( summ[i].altnum == summ[i-1].altnum ) {
                    memset(altnum,' ',6);
                    sprintf(tbuf,"%-6s %-6s %-6s %-6s %-6s %-6s %-6s",
                            ifnum,altnum,epnum,iodirn,iotype,maxps,interval);
                    ui->lw_summ->addItem(QString(tbuf));
                }
                else { sprintf(tbuf,"%-6s %-6s %-6s %-6s %-6s %-6s %-6s",
                            ifnum,altnum,epnum,iodirn,iotype,maxps,interval);
                    ui->lw_summ->addItem("");
                    ui->lw_summ->addItem(tbuf);
                }
            }
            else { sprintf(tbuf,"%-6s %-6s %-6s %-6s %-6s %-6s %-6s",
                        ifnum,altnum,epnum,iodirn,iotype,maxps,interval);
                ui->lw_summ->addItem("");
                ui->lw_summ->addItem(tbuf);
            }
        }
    }
}

void ControlCenter::get_config_details()
{
    int r;
    int i, j, k;
    char tbuf[60];
    char tval[3];
    struct libusb_config_descriptor *desc = NULL;

    h = cyusb_gethandle(current_device_index);

    r = cyusb_get_active_config_descriptor(h, &desc);
    if ( r ) {
        libusb_error(r, "Error getting configuration descriptor");
        return ;
    }
    sprintf(tval,"%d",desc->bNumInterfaces);
    ui->le_numIfaces->setReadOnly(true);
    ui->le_numIfaces->setText(tval);
    ui->sb_selectIf->setMaximum(desc->bNumInterfaces - 1);

    sprintf(tbuf,"<CONFIGURATION>");
    ui->lw_desc->addItem(QString(tbuf));
    sprintf(tbuf,"bLength             = %d",   desc->bLength);
    ui->lw_desc->addItem(QString(tbuf));
    sprintf(tbuf,"bDescriptorType     = %d",   desc->bDescriptorType);
    ui->lw_desc->addItem(QString(tbuf));
    sprintf(tbuf,"TotalLength         = %d",   desc->wTotalLength);
    ui->lw_desc->addItem(QString(tbuf));
    sprintf(tbuf,"Num. of interfaces  = %d",   desc->bNumInterfaces);
    ui->lw_desc->addItem(QString(tbuf));
    sprintf(tbuf,"bConfigurationValue = %d",   desc->bConfigurationValue);
    ui->lw_desc->addItem(QString(tbuf));
    sprintf(tbuf,"iConfiguration      = %d",    desc->iConfiguration);
    ui->lw_desc->addItem(QString(tbuf));
    sprintf(tbuf,"bmAttributes        = %d",    desc->bmAttributes);
    ui->lw_desc->addItem(QString(tbuf));
    sprintf(tbuf,"Max Power           = %04d",  desc->MaxPower);
    ui->lw_desc->addItem(QString(tbuf));

    summ_count = 0;

    for ( i = 0; i < desc->bNumInterfaces; ++i ) {
        const struct libusb_interface *iface = desc->interface;
        ui->cb6_in->clear();
        ui->cb6_out->clear();
        for ( j = 0; j < iface[i].num_altsetting; ++j ) {
            sprintf(tbuf,"\t<INTERFACE>");
            ui->lw_desc->addItem(QString(tbuf));
            const struct libusb_interface_descriptor *ifd = iface[i].altsetting;
            sprintf(tbuf,"\tbLength             = %d",   ifd[j].bLength);
            ui->lw_desc->addItem(QString(tbuf));
            sprintf(tbuf,"\tbDescriptorType     = %d",   ifd[j].bDescriptorType);
            ui->lw_desc->addItem(QString(tbuf));
            sprintf(tbuf,"\tbInterfaceNumber    = %d",   ifd[j].bInterfaceNumber);
            ui->lw_desc->addItem(QString(tbuf));
            sprintf(tbuf,"\tbAlternateSetting   = %d",   ifd[j].bAlternateSetting);
            ui->lw_desc->addItem(QString(tbuf));
            sprintf(tbuf,"\tbNumEndpoints       = %d",   ifd[j].bNumEndpoints);
            ui->lw_desc->addItem(QString(tbuf));
            sprintf(tbuf,"\tbInterfaceClass     = %02x", ifd[j].bInterfaceClass);
            ui->lw_desc->addItem(QString(tbuf));
            sprintf(tbuf,"\tbInterfaceSubClass  = %02x", ifd[j].bInterfaceSubClass);
            ui->lw_desc->addItem(QString(tbuf));
            sprintf(tbuf,"\tbInterfaceProtcol   = %02x", ifd[j].bInterfaceProtocol);
            ui->lw_desc->addItem(QString(tbuf));
            sprintf(tbuf,"\tiInterface          = %0d",  ifd[j].iInterface);
            ui->lw_desc->addItem(QString(tbuf));


            for ( k = 0; k < ifd[j].bNumEndpoints; ++k ) {
                sprintf(tbuf,"\t\t<ENDPOINT>");
                ui->lw_desc->addItem(QString(tbuf));
                const struct libusb_endpoint_descriptor *ep = ifd[j].endpoint;
                struct libusb_ss_endpoint_companion_descriptor *compd = NULL;

                libusb_get_ss_endpoint_companion_descriptor (NULL, ep, &compd);

                sprintf(tbuf,"\t\tbLength             = %0d",  ep[k].bLength);
                ui->lw_desc->addItem(QString(tbuf));
                sprintf(tbuf,"\t\tbDescriptorType     = %0d",  ep[k].bDescriptorType);
                ui->lw_desc->addItem(QString(tbuf));
                sprintf(tbuf,"\t\tbEndpointAddress    = %02x", ep[k].bEndpointAddress);
                ui->lw_desc->addItem(QString(tbuf));
                sprintf(tbuf,"\t\tbmAttributes        = %d",   ep[k].bmAttributes);
                ui->lw_desc->addItem(QString(tbuf));

                summ[summ_count].ifnum    = ifd[j].bInterfaceNumber;
                summ[summ_count].altnum   = ifd[j].bAlternateSetting;
                summ[summ_count].epnum    = ep[k].bEndpointAddress;
                summ[summ_count].eptype   = ep[k].bmAttributes;
                summ[summ_count].maxps    = ep[k].wMaxPacketSize & 0x7ff;  /* ignoring bits 11,12 */
                summ[summ_count].interval = ep[k].bInterval;

                if (compd != NULL) {
                    // USB 3.0. Multiple max packet size by burst size.
                    if (summ[summ_count].eptype == LIBUSB_TRANSFER_TYPE_ISOCHRONOUS)
                        summ[summ_count].reqsize = (summ[summ_count].maxps * (compd->bMaxBurst + 1) *
                                                    (compd->bmAttributes + 1));
                    else
                        summ[summ_count].reqsize = (summ[summ_count].maxps * (compd->bMaxBurst + 1));
                } else {
                    // USB 2.0. Multiply packet size by mult for ISO endpoints
                    if (summ[summ_count].eptype == LIBUSB_TRANSFER_TYPE_ISOCHRONOUS)
                        summ[summ_count].reqsize =
                            cyusb_get_max_iso_packet_size (h, summ[summ_count].epnum);
                    else
                        summ[summ_count].reqsize  = summ[summ_count].maxps;
                }

                ++summ_count;
                sprintf(tbuf,"\t\twMaxPacketSize      = %04x", (ep[k].wMaxPacketSize));
                ui->lw_desc->addItem(QString(tbuf));
                sprintf(tbuf,"\t\tbInterval           = %d",   ep[k].bInterval);
                ui->lw_desc->addItem(QString(tbuf));
                sprintf(tbuf,"\t\tbRefresh            = %d",   ep[k].bRefresh);
                ui->lw_desc->addItem(QString(tbuf));
                sprintf(tbuf,"\t\tbSynchAddress       = %d",   ep[k].bSynchAddress);
                ui->lw_desc->addItem(QString(tbuf));
                sprintf(tbuf,"\t\t</ENDPOINT>");
                ui->lw_desc->addItem(QString(tbuf));
            }

            sprintf(tbuf,"\t</INTERFACE>");
            ui->lw_desc->addItem(QString(tbuf));
        }

    }
    sprintf(tbuf,"</CONFIGURATION>");
    ui->lw_desc->addItem(QString(tbuf));

    cyusb_free_config_descriptor(desc);

    check_for_kernel_driver();
    update_summary();
    on_pb_setIFace_clicked();
}

void ControlCenter::disable_vendor_extensions()
{
    ui->rb3_ramdl->setEnabled(false);
    ui->rb3_ramup->setEnabled(false);
    ui->rb3_eedl->setEnabled(false);
    ui->rb3_eeup->setEnabled(false);
    ui->rb3_getchip->setEnabled(false);
    ui->rb3_renum->setEnabled(false);
}

void ControlCenter::enable_vendor_extensions()
{
    ui->rb3_ramdl->setEnabled(true);
    ui->rb3_ramup->setEnabled(true);
    ui->rb3_eedl->setEnabled(true);
    ui->rb3_eeup->setEnabled(true);
    ui->rb3_getchip->setEnabled(true);
    ui->rb3_renum->setEnabled(true);
    ui->rb3_custom->setChecked(true);
}

void ControlCenter::detect_device(void)
{
    int r;
    unsigned char byte = 0;

    r = cyusb_control_transfer(h, 0xC0, 0xA0, 0xE600, 0x00, &byte, 1, 1000);
    if ( r == 1 ) {
        ui->label_devtype->setText("FX2");
        enable_vendor_extensions();
        ui->tab_4->setEnabled(true);
        ui->tab_5->setEnabled(false);
        ui->tab2->setCurrentIndex(0);
    }
    else {
        ui->label_devtype->setText("FX3");
        disable_vendor_extensions();
        ui->rb3_custom->setChecked(true);
        ui->tab_4->setEnabled(false);
        ui->tab_5->setEnabled(true);
        ui->tab2->setCurrentIndex(1);
    }
}

void ControlCenter::get_device_details()
{
    int r;
    char tbuf[60];
    char tval[3];
    struct libusb_device_descriptor desc;
    struct libusb_config_descriptor *config_desc = NULL;

    h = cyusb_gethandle(current_device_index);
    if ( !h ) {
        printf("Error in getting a handle. curent_device_index = %d\n", current_device_index);
    }

    r = cyusb_get_device_descriptor(h, &desc);
    if ( r ) {
        libusb_error(r, "Error getting device descriptor");
        return ;
    }
    r = cyusb_get_active_config_descriptor(h, &config_desc);
    sprintf(tval,"%d",config_desc->bNumInterfaces);
    ui->le_numIfaces->setText(tval);
    ui->sb_selectIf->setEnabled(true);
    ui->sb_selectIf->setMaximum(config_desc->bNumInterfaces - 1);
    ui->sb_selectIf->setValue(1);
    ui->pb_setIFace->setEnabled(true);

    ui->lw_desc->clear();

    sprintf(tbuf,"<DEVICE>               ");
    ui->lw_desc->addItem(QString(tbuf));

    sprintf(tbuf,"bLength             = %d",   desc.bLength);
    ui->lw_desc->addItem(QString(tbuf));
    sprintf(tbuf,"bDescriptorType     = %d",   desc.bDescriptorType);
    ui->lw_desc->addItem(QString(tbuf));
    sprintf(tbuf,"bcdUSB              = %d",   desc.bcdUSB);
    ui->lw_desc->addItem(QString(tbuf));
    sprintf(tbuf,"bDeviceClass        = %d",   desc.bDeviceClass);
    ui->lw_desc->addItem(QString(tbuf));
    sprintf(tbuf,"bDeviceSubClass     = %d",   desc.bDeviceSubClass);
    ui->lw_desc->addItem(QString(tbuf));
    sprintf(tbuf,"bDeviceProtocol     = %d",   desc.bDeviceProtocol);
    ui->lw_desc->addItem(QString(tbuf));
    sprintf(tbuf,"bMaxPacketSize      = %d",   desc.bMaxPacketSize0);
    ui->lw_desc->addItem(QString(tbuf));
    sprintf(tbuf,"idVendor            = %04x", desc.idVendor);
    ui->lw_desc->addItem(QString(tbuf));
    sprintf(tbuf,"idProduct           = %04x", desc.idProduct);
    ui->lw_desc->addItem(QString(tbuf));
    sprintf(tbuf,"bcdDevice           = %d",   desc.bcdDevice);
    ui->lw_desc->addItem(QString(tbuf));
    sprintf(tbuf,"iManufacturer       = %d",   desc.iManufacturer);
    ui->lw_desc->addItem(QString(tbuf));
    sprintf(tbuf,"iProduct            = %d",   desc.iProduct);
    ui->lw_desc->addItem(QString(tbuf));
    sprintf(tbuf,"iSerialNumber       = %d",   desc.iSerialNumber);
    ui->lw_desc->addItem(QString(tbuf));
    sprintf(tbuf,"bNumConfigurations  = %d",   desc.bNumConfigurations);
    ui->lw_desc->addItem(QString(tbuf));
    sprintf(tbuf,"</DEVICE>               ");
    ui->lw_desc->addItem(QString(tbuf));


    check_for_kernel_driver();
    detect_device();
    on_pb_setIFace_clicked();
    cyusb_free_config_descriptor (config_desc);
}

void ControlCenter::set_if_aif()
{
    int r1, r2;
    char tval[5];

    int i = ui->sb_selectIf->value();
    int a = ui->sb_selectAIf->value();

    r1 = cyusb_claim_interface(h, i);
    if ( r1 == 0 ) {
        r2 = cyusb_set_interface_alt_setting(h, i, a);
        if ( r2 != 0 ) {
            libusb_error(r2, "Error in setting Alternate Interface");
            return;
        }
    }
    else {
        libusb_error(r1, "Error in setting Interface");
        return;
    }
    sprintf(tval,"%d",a);
    ui->label_aif->setText(tval);
}

void ControlCenter::on_listWidget_itemClicked(QListWidgetItem *item)
{
    item = item;
    clear_widgets();
    current_device_index = ui->listWidget->currentRow();
    get_device_details();
    get_config_details();
    set_if_aif();
}

void ControlCenter::sigusr1_handler()
{
    char tmp;
    int  n;

    sn_sigusr1->setEnabled(false);
    n = read(sigusr1_fd[1], &tmp, 1);
    if (n < 0)
        printf ("read returned %d\n", n);

    update_devlist();
    sn_sigusr1->setEnabled(true);
}

void ControlCenter::on_pb1_selfile_clicked()
{
    QString filename;
    if ( (current_device_index == -1) || (ui->label_if->text() == "") || (ui->label_aif->text() == "") ) {
        QMessageBox mb;
        mb.setText("No device+iface+alt-iface has been selected yet !\n");
        mb.exec();
        return ;
    }
    if ( ui->rb1_ram->isChecked() )
        filename = QFileDialog::getOpenFileName(this, "Select file to download...", ".", "Hex file (*.hex);;IIC file (*.iic);;BIN file (*.bin)");
    else
        filename = QFileDialog::getOpenFileName(this, "Select file to download...", ".", "IIC files (*.iic)");
    ui->label1_selfile->setText(filename);
    if ( filename != "" ) {
        ui->pb1_start->setEnabled(true);
    }
}

void ControlCenter::on_pb_reset_clicked()
{
    if ( current_device_index == -1 ) {
        QMessageBox mb;
        mb.setText("No device has been selected yet !\n");
        mb.exec();
        return ;
    }

    cyusb_reset_device(h);
    QMessageBox mb;
    mb.setText("Device reset");
    mb.exec();
}

void ControlCenter::on_pb3_selfile_clicked()
{
    QString filename;
    if ( (current_device_index == -1) || (ui->label_if->text() == "") || (ui->label_aif->text() == "") ) {
        QMessageBox mb;
        mb.setText("No device+iface+alt-iface has been selected yet !\n");
        mb.exec();
        return ;
    }
    filename = QFileDialog::getOpenFileName(this, "Select file to download...", ".", "Image files (*.hex)");
    ui->lab3_selfile->setText(filename);
    if ( filename != "" ) {
        ui->pb3_dl->setEnabled(true);
    }

}

void ControlCenter::on_le6_out_ascii_textEdited()
{
    char tbuf[5];
    unsigned int sz;

    QByteArray t = ui->le6_out_ascii->text().toUtf8();

    sz = (t.size () > 4096) ? 4096 : t.size();
    memcpy (le6_out_data, (const char *)t.data(), sz);

    ui->le6_out_hex->setText("");
    ui->le6_outfile->setText("");

    sprintf(tbuf,"%04d", sz);
    ui->le6_size->setText(tbuf);

}
void ControlCenter::on_le6_out_ascii_textChanged()
{
    char tbuf[5];
    unsigned int sz;

    QByteArray t = ui->le6_out_ascii->text().toUtf8();

    sz = (t.size () > 4096) ? 4096 : t.size();
    memcpy (le6_out_data, (const char *)t.data(), sz);

    ui->le6_out_hex->setText("");
    ui->le6_outfile->setText("");

    sprintf(tbuf,"%04d", sz);
    ui->le6_size->setText(tbuf);
}

void ControlCenter::on_pb6_clear_clicked()
{
    ui->lw6->clear();
    ui->lw6_out->clear();
    ui->le6_out_hex->clear();
    ui->le6_out_ascii->clear();
    ui->label6_out->clear();
    ui->label6_in->clear();
    ui->le6_outfile->clear();
    ui->le6_infile->clear();
    ui->le6_size->clear();
    cum_data_in = cum_data_out = 0;
}

void ControlCenter::on_rb6_constant_clicked()
{
    ui->le6_value->setReadOnly(false);
    ui->le6_size->setText("512");

}

void ControlCenter::on_rb6_random_clicked()
{
    ui->le6_size->setText("512");
    ui->le6_value->setReadOnly(true);
}

void ControlCenter::on_rb6_inc_clicked()
{
    ui->le6_size->setText("512");
    ui->le6_value->setReadOnly(false);
}

void ControlCenter::on_cb6_loop_clicked()
{
    if ( ui->cb6_loop->isChecked() ) {
        ui->pb6_rcv->setEnabled(false);
    }
    else {
        ui->pb6_rcv->setEnabled(true);
    }
}


void ControlCenter::clearhalt_in()
{
    int r;
    unsigned char ep;
    bool ok;

    ep = ui->cb6_in->currentText().toInt(&ok, 16);
    r = cyusb_clear_halt(h, ep);
    if ( r ) {
        libusb_error(r, "Error while automatically clearing halt condition on IN pipe");
        return;
    }
}

void ControlCenter::clearhalt_out()
{
    int r;
    unsigned char ep;
    bool ok;

    ep = ui->cb6_out->currentText().toInt(&ok, 16);
    r = cyusb_clear_halt(h, ep);
    if ( r ) {
        libusb_error(r, "Error while automatically clearing halt condition on OUT pipe");
        return;
    }
}


void ControlCenter::dump_data6_in(int num_bytes, unsigned char *dbuf)
{
    int i, j, k, index, filler;
    char ttbuf[10];
    char finalbuf[256];
    char tbuf[256];
    int balance = 0;

    balance = num_bytes;
    index = 0;

    while ( balance > 0 ) {
        tbuf[0]  = '\0';
        if ( balance < 8 )
            j = balance;
        else j = 8;
        for ( i = 0; i < j; ++i ) {
            sprintf(ttbuf,"%02x ",(unsigned char)dbuf[index+i]);
            strcat(tbuf,ttbuf);
        }
        if ( balance < 8 ) {
            filler = 8-balance;
            for ( k = 0; k < filler; ++k )
                strcat(tbuf,"   ");
        }
        strcat(tbuf,": ");
        for ( i = 0; i < j; ++i ) {
            if ( !isprint(dbuf[index+i]) )
                strcat(tbuf,". ");
            else {
                sprintf(ttbuf,"%c ",dbuf[index+i]);
                strcat(tbuf,ttbuf);
            }
        }
        sprintf(finalbuf,"%s",tbuf);
        ui->lw6->addItem(finalbuf);
        balance -= j;
        index += j;
    }
}

void ControlCenter::dump_data6_out(int num_bytes, unsigned char *dbuf)
{
    int i, j, k, index, filler;
    char ttbuf[10];
    char finalbuf[256];
    char tbuf[256];
    int balance = 0;

    balance = num_bytes;
    index = 0;

    while ( balance > 0 ) {
        tbuf[0]  = '\0';
        if ( balance < 8 )
            j = balance;
        else j = 8;
        for ( i = 0; i < j; ++i ) {
            sprintf(ttbuf,"%02x ",(unsigned char)dbuf[index+i]);
            strcat(tbuf,ttbuf);
        }
        if ( balance < 8 ) {
            filler = 8-balance;
            for ( k = 0; k < filler; ++k )
                strcat(tbuf,"   ");
        }
        strcat(tbuf,": ");
        for ( i = 0; i < j; ++i ) {
            if ( !isprint(dbuf[index+i]) )
                strcat(tbuf,". ");
            else {
                sprintf(ttbuf,"%c ",dbuf[index+i]);
                strcat(tbuf,ttbuf);
            }
        }
        sprintf(finalbuf,"%s",tbuf);
        ui->lw6_out->addItem(finalbuf);
        balance -= j;
        index += j;
    }
}


void ControlCenter::dump_data7_in(int num_bytes, unsigned char *dbuf)
{
    int i, j, k, index, filler;
    char ttbuf[10];
    char finalbuf[256];
    char tbuf[256];
    int balance = 0;

    balance = num_bytes;
    index = 0;

    while ( balance > 0 ) {
        tbuf[0]  = '\0';
        if ( balance < 8 )
            j = balance;
        else j = 8;
        for ( i = 0; i < j; ++i ) {
            sprintf(ttbuf,"%02x ",(unsigned char)dbuf[index+i]);
            strcat(tbuf,ttbuf);
        }
        if ( balance < 8 ) {
            filler = 8-balance;
            for ( k = 0; k < filler; ++k )
                strcat(tbuf,"   ");
        }
        strcat(tbuf,": ");
        for ( i = 0; i < j; ++i ) {
            if ( !isprint(dbuf[index+i]) )
                strcat(tbuf,". ");
            else {
                sprintf(ttbuf,"%c ",dbuf[index+i]);
                strcat(tbuf,ttbuf);
            }
        }
        sprintf(finalbuf,"%s",tbuf);
        ui->lw7_in->addItem(finalbuf);
        balance -= j;
        index += j;
    }
}

void ControlCenter::on_pb6_rcv_clicked()
{
    int r;
    int transferred = 0;
    bool ok;
    unsigned char *buf;
    char tmpbuf[10];

    if ( ui->cb6_loop->isChecked() ) {
        buf = (unsigned char *)malloc(data_count);
        r = cyusb_bulk_transfer(h, ui->cb6_in->currentText().toInt(&ok, 16), buf,
                                data_count, &transferred, 1000);
        printf("Bytes read from device = %d\n",transferred);
        if ( r ) {
            libusb_error(r, "Data Read Error");
            clearhalt_in();
        }
        dump_data6_in(transferred, buf);
        cum_data_in += transferred;
        sprintf(tmpbuf,"%d",cum_data_in);
        ui->label6_in->setText(tmpbuf);
        if ( ui->le6_infile->text() != "" ) {
            r = write(fd_infile, buf, transferred);
            if (r < 0)
                printf ("write returned %d\n", r);
        }
    }
    else {
        buf = (unsigned char *)malloc(ui->le6_size->text().toInt(&ok, 10));
        r = cyusb_bulk_transfer(h, ui->cb6_in->currentText().toInt(&ok, 16), buf,
                                ui->le6_size->text().toInt(&ok, 10), &transferred, 1000);
        printf("Bytes read from device = %d\n",transferred);
        dump_data6_in(transferred, buf);
        cum_data_in += transferred;
        sprintf(tmpbuf,"%d",cum_data_in);
        ui->label6_in->setText(tmpbuf);
        if ( ui->le6_infile->text() != "" ) {
            r = write(fd_infile, buf, transferred);
            if (r < 0)
                printf ("write returned %d\n", r);
        }
    }
    free(buf);
}

void ControlCenter::pb6_send_file_selected(unsigned char *buf, int sz)
{
    int r;
    int transferred = 0;
    bool ok;
    char tmpbuf[10];

    r = cyusb_bulk_transfer(h, ui->cb6_out->currentText().toInt(&ok, 16), buf, sz, &transferred, 1000);
    printf("Bytes sent to device = %d\n",transferred);
    if ( r ) {
        libusb_error(r, "Error in bulk write!");
        clearhalt_out();
        return ;
    }
    cum_data_out += transferred;
    sprintf(tmpbuf,"%d",cum_data_out);
    ui->label6_out->setText(tmpbuf);
    dump_data6_out(transferred, buf);

    if ( ui->cb6_loop->isChecked() ) {
        data_count = transferred;
        on_pb6_rcv_clicked();
    }
}

void ControlCenter::pb6_send_nofile_selected()
{
    int r, i;
    int transferred = 0;
    bool ok;
    unsigned char *buf;
    char tmpbuf[10];

    int sz;
    unsigned char val;

    sz = ui->le6_size->text().toInt(&ok, 10);
    buf = (unsigned char *)malloc(sz);

    if ( (ui->le6_out_hex->text() == "" ) && (ui->le6_out_ascii->text() == "") ) {
        if ( ui->rb6_constant->isChecked() ) {
            val = ui->le6_value->text().toInt(&ok, 16);
            for ( i = 0; i < sz; ++i )
                buf[i] = val;
        }
        else if ( ui->rb6_random->isChecked() ) {
            for ( i = 0; i < sz; ++i )
                buf[i] = random();
        }
        else {
            val = ui->le6_value->text().toInt(&ok, 16);
            for ( i = 0; i < sz; ++i ) {
                if ( i == 0 )
                    buf[i] = val;
                else buf[i] = buf[i-1] + 1;
            }
        }
    }
    else {
        memcpy (buf, le6_out_data, sz);
    }

    cum_data_out = 0;
    cum_data_in  = 0;

    r = cyusb_bulk_transfer(h, ui->cb6_out->currentText().toInt(&ok, 16), buf,
                            sz, &transferred, 1000);

    printf("Bytes sent to device = %d\n",transferred);
    if ( r ) {
        libusb_error(r, "Error in bulk write!\nPerhaps size > device buffer ?");
        clearhalt_out();
    }
    cum_data_out += transferred;
    sprintf(tmpbuf,"%d",cum_data_out);
    ui->label6_out->setText(tmpbuf);

    dump_data6_out(transferred, buf);
    free(buf);

    if ( ui->cb6_loop->isChecked() ) {
        data_count = transferred;
        on_pb6_rcv_clicked();
    }
}

void ControlCenter::on_pb6_clearhalt_out_clicked()
{
    int r;
    unsigned char ep;
    bool ok;

    ep = ui->cb6_out->currentText().toInt(&ok, 16);
    r = cyusb_clear_halt(h, ep);
    if ( r ) {
        libusb_error(r, "Error while clearing halt condition on OUT pipe");
        return;
    }
    QMessageBox mb;
    mb.setText("Halt condition cleared on OUT pipe");
    mb.exec();
    return;
}

void ControlCenter::on_pb6_clearhalt_in_clicked()
{
    int r;
    unsigned char ep;
    bool ok;

    ep = ui->cb6_in->currentText().toInt(&ok, 16);
    r = cyusb_clear_halt(h, ep);
    if ( r ) {
        libusb_error(r, "Error while clearing halt condition on IN pipe");
        return;
    }
    QMessageBox mb;
    mb.setText("Halt condition cleared on IN pipe");
    mb.exec();
    return;
}

void ControlCenter::on_pb7_clearhalt_out_clicked()
{
    int r;
    unsigned char ep;
    bool ok;

    ep = ui->cb7_out->currentText().toInt(&ok, 16);
    r = cyusb_clear_halt(h, ep);
    if ( r ) {
        libusb_error(r, "Error while clearing halt condition on OUT pipe");
        return;
    }
    QMessageBox mb;
    mb.setText("Halt condition cleared on OUT pipe");
    mb.exec();
    return;
}

void ControlCenter::on_pb7_clearhalt_in_clicked()
{
    int r;
    unsigned char ep;
    bool ok;

    ep = ui->cb7_in->currentText().toInt(&ok, 16);
    r = cyusb_clear_halt(h, ep);
    if ( r ) {
        libusb_error(r, "Error while clearing halt condition on IN pipe");
        return;
    }
    QMessageBox mb;
    mb.setText("Halt condition cleared on IN pipe");
    mb.exec();
    return;
}

void ControlCenter::on_pb6_send_clicked()
{
    int nbr;
    unsigned char *buf;
    int maxps = 0;

    if ( ui->le6_outfile->text() == "" )
        pb6_send_nofile_selected();
    else {
        fd_outfile = open(qPrintable(ui->le6_outfile->text()), O_RDONLY);
        if ( fd_outfile < 0 ) {
            QMessageBox mb;
            mb.setText("Output file not found!");
            mb.exec();
            return ;
        }
        if ( ui->le6_infile->text() != "" ) {
            fd_infile = open(qPrintable(ui->le6_infile->text()), O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
            if ( fd_infile < 0 ) {
                QMessageBox mb;
                mb.setText("Input file creation error");
                mb.exec();
                return;
            }
        }

        maxps = 4096;	/* If sending a file, then data is sent in 4096 byte packets */

        buf = (unsigned char *)malloc(maxps);
        while ( (nbr = read(fd_outfile, buf, maxps)) ) {
            pb6_send_file_selected(buf, nbr);
        }
        free(buf);
        ::close(fd_outfile);
        ::close(fd_infile);
    }
}

void ControlCenter::on_pb6_selout_clicked()
{
    QString filename;
    filename = QFileDialog::getOpenFileName(this, "Select file to download...", ".", "Any file (*)");
    if ( filename == ui->le6_infile->text() ) {
        QMessageBox mb;
        mb.setText("Outfile and Infile cannot be the same !!");
        mb.exec();
        return ;
    }
    else ui->le6_outfile->setText(filename);
}

void ControlCenter::on_pb6_selin_clicked()
{
    QString filename;

    if ( ui->le6_outfile->text() == "" ) {
        QMessageBox mb;
        mb.setText("Cannot select infile when outfile is blank !");
        mb.exec();
        return;
    }

    ui->cb6_loop->setChecked(true);	/* Should auto enable this, for data MUST come only from a file out */

    filename = QFileDialog::getSaveFileName(this, "Select file to write data to...", ".", "Any file (*)");
    if ( filename == ui->le6_outfile->text() ) {
        QMessageBox mb;
        mb.setText("Outfile and Infile cannot be the same !!");
        mb.exec();
        return ;
    }
    else ui->le6_infile->setText(filename);
}

void ControlCenter::on_pb7_clear_clicked()
{
    ui->label7_pktsize_out->clear();
    ui->label7_pktsize_in->clear();
    ui->label7_totalout->clear();
    ui->label7_totalin->clear();
    ui->label7_pktsout->clear();
    ui->label7_pktsin->clear();
    ui->label7_dropped_out->clear();
    ui->label7_dropped_in->clear();
    ui->label7_rateout->clear();
    ui->label7_ratein->clear();
    ui->lw7_out->clear();
    ui->lw7_in->clear();
}


void ControlCenter::on_pb7_rcv_clicked()
{
    int numpkts;
    bool ok;
    int pktsize_in;
    int bufsize_in;
    unsigned char ep_in;
    int r;
    char tbuf[10];
    struct timeval tv;

    if ( ui->cb7_in->currentText() == "" ) {  /* No ep_in exists */
        QMessageBox mb;
        mb.setText("Cannot receive data when no input endpoint present");
        mb.exec();
        return;
    }

    ep_in = ui->cb7_in->currentText().toInt(&ok, 16);
    pktsize_in = cyusb_get_max_iso_packet_size(h, ep_in);
    sprintf(tbuf,"%9d",pktsize_in);
    ui->label7_pktsize_in->setText(tbuf);

    numpkts = ui->cb7_numpkts->currentText().toInt(&ok, 10);
    bufsize_in = pktsize_in * numpkts;
    isoc_databuf = (unsigned char *)malloc(bufsize_in);

    transfer = libusb_alloc_transfer(numpkts);
    if ( !transfer ) {
        QMessageBox mb;
        mb.setText("Alloc failure");
        mb.exec();
        return;
    }
    transfer->user_data = this;

    libusb_fill_iso_transfer(transfer, h, ep_in, isoc_databuf, bufsize_in, numpkts, [](struct libusb_transfer* transfer){
        ((ControlCenter*)transfer->user_data)->in_callback(transfer);
    }, NULL, 10000 );

    libusb_set_iso_packet_lengths(transfer, pktsize_in);
    transfer->flags = LIBUSB_TRANSFER_FREE_BUFFER | LIBUSB_TRANSFER_FREE_TRANSFER;

    isoc_time = new QElapsedTimer();
    isoc_time->start();

    r = libusb_submit_transfer(transfer);

    tv.tv_sec = 0;
    tv.tv_usec = 50;

    for ( int i = 0; i < 100; ++i ) {
        libusb_handle_events_timeout(NULL,&tv);
    }

    if ( r ) {
        printf("Error %d submitting transfer\n", r);
    }
}

void ControlCenter::dump_data7_out(int num_bytes, unsigned char *dbuf)
{
    int i, j, k, index, filler;
    char ttbuf[10];
    char finalbuf[256];
    char tbuf[256];
    int balance = 0;

    balance = num_bytes;
    index = 0;

    while ( balance > 0 ) {
        tbuf[0]  = '\0';
        if ( balance < 8 )
            j = balance;
        else j = 8;
        for ( i = 0; i < j; ++i ) {
            sprintf(ttbuf,"%02x ",(unsigned char)dbuf[index+i]);
            strcat(tbuf,ttbuf);
        }
        if ( balance < 8 ) {
            filler = 8-balance;
            for ( k = 0; k < filler; ++k )
                strcat(tbuf,"   ");
        }
        strcat(tbuf,": ");
        for ( i = 0; i < j; ++i ) {
            if ( !isprint(dbuf[index+i]) )
                strcat(tbuf,". ");
            else {
                sprintf(ttbuf,"%c ",dbuf[index+i]);
                strcat(tbuf,ttbuf);
            }
        }
        sprintf(finalbuf,"%s",tbuf);
        ui->lw7_out->addItem(finalbuf);
        balance -= j;
        index += j;
    }
}


void ControlCenter::in_callback( struct libusb_transfer *transfer)
{
    bool ok;
    int pktsin, act_len;
    char tbuf[8];
    unsigned char *ptr;
    int elapsed;
    unsigned char ep_in;
    int pktsize_in;
    char ttbuf[10];
    double inrate;

    printf("Callback function called\n");

    if ( transfer->status != LIBUSB_TRANSFER_COMPLETED ) {
        libusb_error(transfer->status, "Transfer not completed normally");
    }

    totalin = pkts_success = pkts_failure = 0;

    pktsin = ui->cb7_numpkts->currentText().toInt(&ok, 10);

    elapsed = isoc_time->elapsed();

    printf("Milliseconds elapsed = %d\n",elapsed);

    for ( int i = 0; i < pktsin; ++i ) {
        ptr = libusb_get_iso_packet_buffer_simple(transfer, i);
        act_len = transfer->iso_packet_desc[i].actual_length;
        totalin += 1;
        if ( transfer->iso_packet_desc[i].status == LIBUSB_TRANSFER_COMPLETED ) {
            pkts_success += 1;
            if ( ui->rb7_enable->isChecked() ) {
                dump_data7_in(act_len, ptr);
                ui->lw7_in->addItem("");
            }
        }
        else pkts_failure += 1;
    }
    sprintf(tbuf,"%6d",totalin);
    ui->label7_totalin->setText(tbuf);
    sprintf(tbuf,"%6d",pkts_success);
    ui->label7_pktsin->setText(tbuf);
    sprintf(tbuf,"%6d",pkts_failure);
    ui->label7_dropped_in->setText(tbuf);
    ep_in = ui->cb7_in->currentText().toInt(&ok, 16);
    pktsize_in = cyusb_get_max_iso_packet_size(h, ep_in);
    inrate = ( (((double)totalin * (double)pktsize_in) / (double)elapsed ) * (1000.0 / 1024.0) );
    sprintf(ttbuf, "%8.1f", inrate);
    ui->label7_ratein->setText(ttbuf);
}

void ControlCenter::out_callback( struct libusb_transfer *transfer)
{
    bool ok;
    int pktsout, act_len;
    char tbuf[8];
    unsigned char *ptr;
    int elapsed;
    unsigned char ep_out;
    int pktsize_out;
    char ttbuf[10];
    double outrate;


    printf("Callback function called\n");

    if ( transfer->status != LIBUSB_TRANSFER_COMPLETED ) {
        libusb_error(transfer->status, "Transfer not completed normally");
    }

    totalout = pkts_success = pkts_failure = 0;

    pktsout = ui->cb7_numpkts->currentText().toInt(&ok, 10);

    elapsed = isoc_time->elapsed();

    printf("Milliseconds elapsed = %d\n",elapsed);

    for ( int i = 0; i < pktsout; ++i ) {
        ptr = libusb_get_iso_packet_buffer_simple(transfer, i);
        act_len = transfer->iso_packet_desc[i].actual_length;
        totalout += 1;
        if ( transfer->iso_packet_desc[i].status == LIBUSB_TRANSFER_COMPLETED ) {
            pkts_success += 1;
            if ( ui->rb7_enable->isChecked() ) {
                dump_data7_out(act_len, ptr);
                ui->lw7_out->addItem("");
            }
        }
        else pkts_failure += 1;
    }
    sprintf(tbuf,"%6d",totalout);
    ui->label7_totalout->setText(tbuf);
    sprintf(tbuf,"%6d",pkts_success);
    ui->label7_pktsout->setText(tbuf);
    sprintf(tbuf,"%6d",pkts_failure);
    ui->label7_dropped_out->setText(tbuf);
    ep_out = ui->cb7_out->currentText().toInt(&ok, 16);
    pktsize_out = cyusb_get_max_iso_packet_size(h, ep_out);
    outrate = ( (((double)totalout * (double)pktsize_out) / (double)elapsed ) * (1000.0 / 1024.0) );
    sprintf(ttbuf, "%8.1f", outrate);
    ui->label7_rateout->setText(ttbuf);
}


void ControlCenter::on_pb7_send_clicked()
{
    int numpkts;
    bool ok;
    int pktsize_out;
    int bufsize_out;
    unsigned char ep_out;
    int r;
    char tbuf[10];
    struct timeval tv;

    if ( ui->cb7_out->currentText() == "" ) {  /* No ep_out exists */
        QMessageBox mb;
        mb.setText("Cannot send data when no output endpoint present");
        mb.exec();
        return;
    }

    ep_out = ui->cb7_out->currentText().toInt(&ok, 16);
    pktsize_out = cyusb_get_max_iso_packet_size(h, ep_out);
    sprintf(tbuf,"%9d",pktsize_out);
    ui->label7_pktsize_out->setText(tbuf);

    numpkts = ui->cb7_numpkts->currentText().toInt(&ok, 10);
    bufsize_out = pktsize_out * numpkts;
    isoc_databuf = (unsigned char *)malloc(bufsize_out);

    for ( int i = 0; i < numpkts; ++i )
        for ( int j = 0; j < pktsize_out; ++j )
            isoc_databuf[i*pktsize_out+j] = i + 1;

    transfer = libusb_alloc_transfer(numpkts);
    if ( !transfer ) {
        QMessageBox mb;
        mb.setText("Alloc failure");
        mb.exec();
        return;
    }
    transfer->user_data = this;

    libusb_fill_iso_transfer(transfer, h, ep_out, isoc_databuf, bufsize_out, numpkts, [](struct libusb_transfer* transfer) {
        ((ControlCenter*)transfer->user_data)->out_callback(transfer);
    }, NULL, 10000 );

    libusb_set_iso_packet_lengths(transfer, pktsize_out);
    transfer->flags = LIBUSB_TRANSFER_FREE_BUFFER | LIBUSB_TRANSFER_FREE_TRANSFER;

    isoc_time = new QElapsedTimer();
    isoc_time->start();

    r = libusb_submit_transfer(transfer);


    tv.tv_sec = 0;
    tv.tv_usec = 50;

    for ( int i = 0; i < 100; ++i ) {
        libusb_handle_events_timeout(NULL,&tv);
    }

    if ( r ) {
        printf("Error %d submitting transfer\n", r);
    }
}

void ControlCenter::on_rb7_enable_clicked()
{
    ui->lw7_out->setEnabled(true);
    ui->lw7_in->setEnabled(true);
}

void ControlCenter::on_rb7_disable_clicked()
{
    ui->lw7_out->setEnabled(false);
    ui->lw7_in->setEnabled(false);
}

void ControlCenter::appExit()
{
    exit(0);
}

void ControlCenter::about()
{
    QMessageBox mb;
    mb.setText("CyUSB Suite for Linux - Version 1.0");
    mb.setDetailedText("(c) Cypress Semiconductor Corporation, ATR-LABS - 2012");
    mb.exec();
    return;
}

void ControlCenter::set_tool_tips(void)
{
    ui->cb_kerneldriver->setToolTip("If checked, implies interface has been claimed");
    ui->pb_kerneldetach->setToolTip("Releases a claimed interface");
    ui->le3_wval->setToolTip("Enter address for A2/A3, perhaps Custom commands");
}

void ControlCenter::setup_handler(int signo)
{
    char a = 1;
    int N;

    printf("Signal %d (=SIGUSR1) received !\n",signo);
    cyusb_close();
    N = cyusb_open();
    if ( N < 0 ) {
        printf("Error in opening library\n");
        exit(-1);
    }
    else if ( N == 0 ) {
        printf("No device of interest found\n");
        num_devices_detected = 0;
        current_device_index = -1;
    }
    else {
        printf("No of devices of interest found = %d\n",N);
        num_devices_detected = N;
        current_device_index = 0;
    }

    N = write(sigusr1_fd[0], &a, 1);
    if (N < 0)
        printf ("write returned %d\n", N);
}
