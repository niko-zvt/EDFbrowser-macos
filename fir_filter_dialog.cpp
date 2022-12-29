/*
***************************************************************************
*
* Author: Teunis van Beelen
*
* Copyright (C) 2018 - 2022 Teunis van Beelen
*
* Email: teuniz@protonmail.com
*
***************************************************************************
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, version 3 of the License.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
***************************************************************************
*/



#include "fir_filter_dialog.h"


#define FIR_FILTER_MAX_MEAS_PNTS     (10000)



UI_FIRFilterDialog::UI_FIRFilterDialog(char *recent_dir, char *save_dir, QWidget *w_parent)
{
  int i, n;

  QListWidgetItem *item;

  QList<QListWidgetItem *> selectedlist;

  n_taps = 0;

  db_max_i = 10;
  db_min_i = -100;

  plot_phase = 0;

  z = NULL;
  deg = NULL;

  mainwindow = (UI_Mainwindow *)w_parent;

  recent_opendir = recent_dir;
  recent_savedir = save_dir;

  firfilterdialog = new QDialog;
  firfilterdialog->setMinimumSize(800 * mainwindow->w_scaling, 400 * mainwindow->h_scaling);
  firfilterdialog->setWindowTitle("Add a Custom FIR filter");
  firfilterdialog->setModal(true);
  firfilterdialog->setAttribute(Qt::WA_DeleteOnClose, true);
  firfilterdialog->setSizeGripEnabled(true);

  varsLabel = new QLabel;
  varsLabel->setText("Filter taps: 0");

  freqresplabel = new QLabel;
  freqresplabel->setText("Frequency response: 0Hz - Fs/2 (Nyquist)");

  listlabel = new QLabel;
  listlabel->setText("Select signals:");

  textEdit = new QPlainTextEdit;
  textEdit->setReadOnly(false);
  textEdit->setToolTip("Copy & paste your filter coefficients here");

  list = new QListWidget;
  list->setSelectionBehavior(QAbstractItemView::SelectRows);
  list->setSelectionMode(QAbstractItemView::ExtendedSelection);

  curve1 = new FilterCurve;
  curve1->setH_RasterSize(10);
  curve1->setH_Range(0, 0.5);
  curve1->setH_Range_Enabled(true);
  curve1->setV_RasterSize(11);
  curve1->setV_Range(10, -100);
  curve1->setV_Range_Enabled(true);

  db_max_spinbox = new QSpinBox;
  db_max_spinbox->setRange(-200, 200);
  db_max_spinbox->setSingleStep(10);
  db_max_spinbox->setPrefix("Max ");
  db_max_spinbox->setSuffix(" dB");
  db_max_spinbox->setValue(10);
  db_max_spinbox->setEnabled(false);

  db_min_spinbox = new QSpinBox;
  db_min_spinbox->setRange(-200, 200);
  db_min_spinbox->setSingleStep(10);
  db_min_spinbox->setPrefix("Min ");
  db_min_spinbox->setSuffix(" dB");
  db_min_spinbox->setValue(-100);
  db_min_spinbox->setEnabled(false);

  plotphasecheckbox = new QCheckBox("Phase");
  plotphasecheckbox->setCheckState(Qt::Unchecked);
  plotphasecheckbox->setTristate(false);
  plotphasecheckbox->setEnabled(false);

  CancelButton = new QPushButton;
  CancelButton->setText("&Close");

  fileButton = new QPushButton;
  fileButton->setText("&File");
  fileButton->setToolTip("Load filter coefficients from a text file");

  ApplyButton = new QPushButton;
  ApplyButton->setText("&Apply");
  ApplyButton->setToolTip("Apply the current filter to the selected (highlighted) signals");

  helpButton = new QPushButton;
  helpButton->setText("Help");

  for(i=0; i<mainwindow->signalcomps; i++)
  {
    item = new QListWidgetItem;
    if(mainwindow->signalcomp[i]->alias[0] != 0)
    {
      item->setText(mainwindow->signalcomp[i]->alias);
    }
    else
    {
      item->setText(mainwindow->signalcomp[i]->signallabel);
    }
    item->setData(Qt::UserRole, QVariant(i));
    list->addItem(item);
  }

  n = list->count();

  for(i=0; i<n; i++)
  {
    item = list->item(i);

    item->setSelected(true);
  }

  selected_signals_changed();

  QVBoxLayout *vlayout2 = new QVBoxLayout;
  vlayout2->addWidget(varsLabel);
  vlayout2->addWidget(textEdit, 1000);

  QHBoxLayout *hlayout3 = new QHBoxLayout;
  hlayout3->addStretch(100);
  hlayout3->addWidget(db_max_spinbox);
  hlayout3->addStretch(100);
  hlayout3->addWidget(db_min_spinbox);
  hlayout3->addStretch(100);
  hlayout3->addWidget(plotphasecheckbox);
  hlayout3->addStretch(1000);

  QVBoxLayout *vlayout3 = new QVBoxLayout;
  vlayout3->addWidget(freqresplabel);
  vlayout3->addWidget(curve1, 1000);
  vlayout3->addLayout(hlayout3);

  QVBoxLayout *vlayout4 = new QVBoxLayout;
  vlayout4->addWidget(listlabel);
  vlayout4->addWidget(list, 1000);

  QHBoxLayout *hlayout2 = new QHBoxLayout;
  hlayout2->addLayout(vlayout2, 1);
  hlayout2->addLayout(vlayout3, 2);
  hlayout2->addLayout(vlayout4, 1);

  QHBoxLayout *hlayout1 = new QHBoxLayout;
  hlayout1->addWidget(CancelButton);
  hlayout1->addStretch(1000);
  hlayout1->addWidget(fileButton);
  hlayout1->addStretch(1000);
  hlayout1->addWidget(ApplyButton);
  hlayout1->addStretch(1000);
  hlayout1->addWidget(helpButton);

  QVBoxLayout *vlayout1 = new QVBoxLayout;
  vlayout1->addLayout(hlayout2, 1000);
  vlayout1->addSpacing(20);
  vlayout1->addLayout(hlayout1);

  firfilterdialog->setLayout(vlayout1);

  z   = (double *)malloc(sizeof(double) * FIR_FILTER_MAX_MEAS_PNTS);
  deg = (double *)malloc(sizeof(double) * FIR_FILTER_MAX_MEAS_PNTS);

  QObject::connect(ApplyButton,       SIGNAL(clicked()),              this,            SLOT(ApplyButtonClicked()));
  QObject::connect(CancelButton,      SIGNAL(clicked()),              firfilterdialog, SLOT(close()));
  QObject::connect(textEdit,          SIGNAL(textChanged()),          this,            SLOT(check_text()));
  QObject::connect(helpButton,        SIGNAL(clicked()),              this,            SLOT(helpbuttonpressed()));
  QObject::connect(fileButton,        SIGNAL(clicked()),              this,            SLOT(filebuttonpressed()));
  QObject::connect(db_max_spinbox,    SIGNAL(valueChanged(int)),      this,            SLOT(db_max_spinbox_changed(int)));
  QObject::connect(db_min_spinbox,    SIGNAL(valueChanged(int)),      this,            SLOT(db_min_spinbox_changed(int)));
  QObject::connect(plotphasecheckbox, SIGNAL(stateChanged(int)),      this,            SLOT(plotphasecheckbox_changed(int)));
  QObject::connect(list,              SIGNAL(itemSelectionChanged()), this,            SLOT(selected_signals_changed()));


  firfilterdialog->exec();
}


UI_FIRFilterDialog::~UI_FIRFilterDialog()
{
  free(z);
  free(deg);
}


void UI_FIRFilterDialog::plotphasecheckbox_changed(int status)
{
  if(n_taps < 2)  return;

  if(status == Qt::Checked)
  {
    plot_phase = 1;

    curve1->drawCurve(z, FIR_FILTER_MAX_MEAS_PNTS, db_max_i, db_min_i, deg, FIR_FILTER_MAX_MEAS_PNTS, 180, -180, 0, 0);
  }
  else
  {
    plot_phase = 0;

    curve1->drawCurve(z, FIR_FILTER_MAX_MEAS_PNTS, db_max_i, db_min_i);
  }
}


void UI_FIRFilterDialog::db_max_spinbox_changed(int val)
{
  db_max_i = val;

  curve1->setV_RasterSize((db_max_i / 10) + (db_min_i / -10));

  curve1->setV_Range(db_max_i, db_min_i);

  if(n_taps < 2)
  {
    curve1->update();

    return;
  }

  if(plot_phase)
  {
    curve1->drawCurve(z, FIR_FILTER_MAX_MEAS_PNTS, db_max_i, db_min_i, deg, FIR_FILTER_MAX_MEAS_PNTS, 180, -180, 0, 0);
  }
  else
  {
    curve1->drawCurve(z, FIR_FILTER_MAX_MEAS_PNTS, db_max_i, db_min_i);
  }
}


void UI_FIRFilterDialog::db_min_spinbox_changed(int val)
{
  db_min_i = val;

  curve1->setV_RasterSize((db_max_i / 10) + (db_min_i / -10));

  curve1->setV_Range(db_max_i, db_min_i);

  if(n_taps < 2)
  {
    curve1->update();

    return;
  }

  if(plot_phase)
  {
    curve1->drawCurve(z, FIR_FILTER_MAX_MEAS_PNTS, db_max_i, db_min_i, deg, FIR_FILTER_MAX_MEAS_PNTS, 180, -180, 0, 0);
  }
  else
  {
    curve1->drawCurve(z, FIR_FILTER_MAX_MEAS_PNTS, db_max_i, db_min_i);
  }
}


void UI_FIRFilterDialog::check_text()
{
  int i, len=0, err;

  double db_max=-10000, db_min=10000;

  char *str=NULL,
       str2[512]="";

  n_taps = 0;

  strncpy(textbuf, textEdit->toPlainText().toLatin1().data(), FIR_FILTER_MAX_BUFSZ);

  len = strlen(textbuf);

  for(i=0; i<len; i++)
  {
    if(textbuf[i] == ',')
    {
      textbuf[i] = '.';
    }
  }

  textbuf[FIR_FILTER_MAX_BUFSZ] = 0;

  str = strtok(textbuf, "\r\n");

  while(str != NULL)
  {
    taps[n_taps++] = atof(str);

    if(n_taps >= FIR_FILTER_MAX_TAPS)  break;

    str = strtok(NULL, "\r\n");
  }

  snprintf(str2, 256, "Filter taps: %i", n_taps);

  varsLabel->setText(str2);

  if(n_taps < 2)
  {
    curve1->clear();
    db_max_spinbox->setEnabled(false);
    db_min_spinbox->setEnabled(false);
    plotphasecheckbox->setEnabled(false);
    return;
  }

  err = fir_filter_freqz(taps, n_taps, z, deg, FIR_FILTER_MAX_MEAS_PNTS);
  if(err)
  {
    db_max_spinbox->setEnabled(false);
    db_min_spinbox->setEnabled(false);
    plotphasecheckbox->setEnabled(false);

    snprintf(str2, 512, "fir_filter_freqz() error: %i", err);
    QMessageBox::critical(firfilterdialog, "Error", str2);
    return;
  }

  db_max_spinbox->setEnabled(true);
  db_min_spinbox->setEnabled(true);
  plotphasecheckbox->setEnabled(true);

  for(i=0; i<FIR_FILTER_MAX_MEAS_PNTS; i++)
  {
    if(z[i] > 1e-50)
    {
      z[i] = -20 * log10(1/z[i]);
    }
    else  /* avoid divide by zero */
    {
      z[i] = -1000;
    }

    if(db_max < z[i])  db_max = z[i];
    if(db_min > z[i])  db_min = z[i];
  }

  db_max_i = (((int)(db_max / 10)) * 10) + 10;
  db_min_i = (((int)(db_min / 10)) * 10) - 10;

  if(db_max_i > 200)  db_max_i = 200;
  if(db_min_i < -200)  db_min_i = -200;

  curve1->setH_RasterSize(10);
  curve1->setV_RasterSize((db_max_i / 10) + (db_min_i / -10));
  if(plot_phase)
  {
    curve1->drawCurve(z, FIR_FILTER_MAX_MEAS_PNTS, db_max_i, db_min_i, deg, FIR_FILTER_MAX_MEAS_PNTS, 180, -180, 0, 0);
  }
  else
  {
    curve1->drawCurve(z, FIR_FILTER_MAX_MEAS_PNTS, db_max_i, db_min_i);
  }

  db_max_spinbox->setValue(db_max_i);
  db_min_spinbox->setValue(db_min_i);
}


void UI_FIRFilterDialog::ApplyButtonClicked()
{
  int i, s, n;

  QListWidgetItem *item;

  QList<QListWidgetItem *> selectedlist;

  selectedlist = list->selectedItems();

  n = selectedlist.size();

  if(n_taps < 2)
  {
    QMessageBox::critical(firfilterdialog, "Error", "Put at least two taps into the list.");
    return;
  }

  for(i=0; i<n; i++)
  {
    item = selectedlist.at(i);
    s = item->data(Qt::UserRole).toInt();

    if(mainwindow->signalcomp[s]->fir_filter)
    {
      free_fir_filter(mainwindow->signalcomp[s]->fir_filter);

      mainwindow->signalcomp[s]->fir_filter = NULL;
    }

    mainwindow->signalcomp[s]->fir_filter = create_fir_filter(taps, n_taps);
    if(mainwindow->signalcomp[s]->fir_filter == NULL)
    {
      QMessageBox::critical(firfilterdialog, "Error", "An error occurred while creating a FIR filter.");
    }
  }

  mainwindow->setup_viewbuf();
}


void UI_FIRFilterDialog::filebuttonpressed()
{
  int len=0, i;

  char path[MAX_PATH_LENGTH]="",
  str[4096]="";

  FILE *inputfile;

  strlcpy(path, QFileDialog::getOpenFileName(0, "Select inputfile", QString::fromLocal8Bit(recent_opendir), "All files (*)").toLocal8Bit().data(), MAX_PATH_LENGTH);

  if(!strcmp(path, ""))
  {
    return;
  }

  get_directory_from_path(recent_opendir, path, MAX_PATH_LENGTH);

  inputfile = fopeno(path, "rb");
  if(inputfile==NULL)
  {
    snprintf(str, 2048, "Can not open file %s for reading.", path);
    QMessageBox messagewindow(QMessageBox::Critical, "Error", QString::fromLocal8Bit(str));
    messagewindow.exec();
    return;
  }

  fseek(inputfile, 0, SEEK_END);

  len = ftell(inputfile);

  if(len < 2)
  {
    QMessageBox::critical(firfilterdialog, "Error", "File too small.");
    fclose(inputfile);
    return;
  }

  if(len > FIR_FILTER_MAX_BUFSZ)  len = FIR_FILTER_MAX_BUFSZ;

  fseek(inputfile, 0, SEEK_SET);

  if(fread(textbuf, len, 1, inputfile) != 1)
  {
    QMessageBox::critical(firfilterdialog, "Error", "Read error.");
    fclose(inputfile);
    return;
  }

  fclose(inputfile);

  for(i=0; i<len; i++)
  {
    if(((textbuf[i] < 32) || (textbuf[i] > 126)) && (textbuf[i] != '\n') && (textbuf[i] != '\r'))
    {
      textbuf[i] = '!';
    }

    if(textbuf[i] == ',')
    {
      textbuf[i] = '.';
    }
  }

  textbuf[len] = 0;

  textEdit->setPlainText(textbuf);
}


void UI_FIRFilterDialog::helpbuttonpressed()
{
#ifdef Q_OS_LINUX
  QDesktopServices::openUrl(QUrl("file:///usr/share/doc/edfbrowser/manual.html#Custom_fir_filter"));
#endif

#ifdef Q_OS_WIN32
  char p_path[MAX_PATH_LENGTH];

  strlcpy(p_path, "file:///", MAX_PATH_LENGTH);
  strlcat(p_path, mainwindow->specialFolder(CSIDL_PROGRAM_FILES).toLocal8Bit().data(), MAX_PATH_LENGTH);
  strlcat(p_path, "\\EDFbrowser\\manual.html#Custom_fir_filter", MAX_PATH_LENGTH);
  QDesktopServices::openUrl(QUrl(p_path));
#endif
}


void UI_FIRFilterDialog::selected_signals_changed()
{
  int n;

  char str[128]={""};

  QList<QListWidgetItem *> selectedlist;

  selectedlist = list->selectedItems();

  n = selectedlist.size();

  snprintf(str, 128, "Selected signals: %i", n);

  listlabel->setText(str);
}
















