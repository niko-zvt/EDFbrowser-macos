/*
***************************************************************************
*
* Author: Teunis van Beelen
*
* Copyright (C) 2018 - 2023 Teunis van Beelen
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
#define FIR_FILTER_MAX_TAPS           (5000)
#define FIR_FILTER_MAX_BUFSZ        (100000)



UI_FIRFilterDialog::UI_FIRFilterDialog(char *recent_dir, char *save_dir, QWidget *w_parent)
{
  int i, n, max_len=4;

  QListWidgetItem *item;

  QList<QListWidgetItem *> selectedlist;

  thrd = NULL;

  n_taps = 0;

  db_max_i = 10;
  db_min_i = -100;

  plot_phase = 0;

  z = NULL;
  deg = NULL;
  textbuf = NULL;
  taps=NULL;

  mainwindow = (UI_Mainwindow *)w_parent;

  recent_opendir = recent_dir;
  recent_savedir = save_dir;

  firfilterdialog = new QDialog;
  firfilterdialog->setMinimumSize(800 * mainwindow->w_scaling, 600 * mainwindow->h_scaling);
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
  list->setMinimumWidth(100 * mainwindow->w_scaling);

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
  fileButton->setText("Load from file");
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

      if(max_len < (int)strlen(mainwindow->signalcomp[i]->alias))
      {
        max_len = strlen(mainwindow->signalcomp[i]->alias);
      }
    }
    else
    {
      item->setText(mainwindow->signalcomp[i]->signallabel);

      if(max_len < (int)strlen(mainwindow->signalcomp[i]->signallabel))
      {
        max_len = strlen(mainwindow->signalcomp[i]->signallabel);
      }
    }
    item->setData(Qt::UserRole, QVariant(i));
    list->addItem(item);
  }

  list->setMinimumWidth(max_len * 8 * mainwindow->w_scaling);

  n = list->count();

  for(i=0; i<n; i++)
  {
    item = list->item(i);

    item->setSelected(true);
  }

  selected_signals_changed();

///////////////////////////////////////////////////////////////////////////////////////////////////////

  tabholder = new QTabWidget;

  pm_tab = new QWidget;

  pm_param_table = new QTableWidget;
  pm_param_table->setSelectionMode(QAbstractItemView::NoSelection);
  pm_param_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
  pm_param_table->setColumnCount(5);
  QStringList pm_horizontallabels;
  pm_horizontallabels += "From";
  pm_horizontallabels += "To";
  pm_horizontallabels += "Gain";
  pm_horizontallabels += "Ripple/Att.";
  pm_horizontallabels += "";
  pm_param_table->setHorizontalHeaderLabels(pm_horizontallabels);
  pm_param_table->resizeColumnsToContents();
  pm_param_table->setMinimumWidth(450 * mainwindow->w_scaling);

  pm_add_passband_button = new QPushButton;
  pm_add_passband_button->setText("Add passband");

  pm_add_stopband_button = new QPushButton;
  pm_add_stopband_button->setText("Add stopband");

  pm_predefined_cbox = new QComboBox;
  pm_predefined_cbox->addItem("No predefined");
  pm_predefined_cbox->addItem("Lowpass");
  pm_predefined_cbox->addItem("Highpass");
  pm_predefined_cbox->addItem("Bandpass");
  pm_predefined_cbox->addItem("Bandstop");

  pm_sf_spinbox = new QDoubleSpinBox;
  pm_sf_spinbox->setRange(1e-3, 1e6);
  pm_sf_spinbox->setPrefix("Fs: ");
  pm_sf_spinbox->setSuffix(" Hz");
  pm_sf_spinbox->setToolTip("Samplefrequency");
  if(mainwindow->signalcomps)
  {
    pm_sf_spinbox->setValue(mainwindow->signalcomp[0]->edfhdr->edfparam[0].sf_f);
  }
  else
  {
    pm_sf_spinbox->setValue(2000);
  }

  pm_design_button = new QPushButton;
  pm_design_button->setText("Design");
  pm_design_button->setEnabled(false);

  QHBoxLayout *hlayout4 = new QHBoxLayout;
  hlayout4->addWidget(pm_param_table);
  hlayout4->addStretch(1);

  QHBoxLayout *hlayout5 = new QHBoxLayout;
  hlayout5->addWidget(pm_predefined_cbox);
  hlayout5->addStretch(10);
  hlayout5->addWidget(pm_add_passband_button);
  hlayout5->addStretch(10);
  hlayout5->addWidget(pm_add_stopband_button);
  hlayout5->addStretch(10);
  hlayout5->addWidget(pm_sf_spinbox);
  hlayout5->addStretch(10);
  hlayout5->addWidget(pm_design_button);
  hlayout5->addStretch(100);

  QVBoxLayout *vlayout5 = new QVBoxLayout;
  vlayout5->addLayout(hlayout4);
  vlayout5->addSpacing(20);
  vlayout5->addLayout(hlayout5);

  pm_tab->setLayout(vlayout5);

  tabholder->addTab(pm_tab, "Parks-McClellan");

///////////////////////////////////////////////////////////////////////////////////////////////////////

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
  hlayout2->addLayout(vlayout3, 100);
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
  vlayout1->addWidget(tabholder);
  vlayout1->addSpacing(40);
  vlayout1->addLayout(hlayout1);

  firfilterdialog->setLayout(vlayout1);

  z   = (double *)malloc(sizeof(double) * FIR_FILTER_MAX_MEAS_PNTS);
  deg = (double *)malloc(sizeof(double) * FIR_FILTER_MAX_MEAS_PNTS);

  taps = (double *)malloc(sizeof(double) * FIR_FILTER_MAX_TAPS);

  textbuf = (char *)malloc(FIR_FILTER_MAX_BUFSZ + 128);

  QObject::connect(ApplyButton,            SIGNAL(clicked()),                this,            SLOT(ApplyButtonClicked()));
  QObject::connect(CancelButton,           SIGNAL(clicked()),                firfilterdialog, SLOT(close()));
  QObject::connect(textEdit,               SIGNAL(textChanged()),            this,            SLOT(check_text()));
  QObject::connect(helpButton,             SIGNAL(clicked()),                this,            SLOT(helpbuttonpressed()));
  QObject::connect(fileButton,             SIGNAL(clicked()),                this,            SLOT(filebuttonpressed()));
  QObject::connect(db_max_spinbox,         SIGNAL(valueChanged(int)),        this,            SLOT(db_max_spinbox_changed(int)));
  QObject::connect(db_min_spinbox,         SIGNAL(valueChanged(int)),        this,            SLOT(db_min_spinbox_changed(int)));
  QObject::connect(plotphasecheckbox,      SIGNAL(stateChanged(int)),        this,            SLOT(plotphasecheckbox_changed(int)));
  QObject::connect(list,                   SIGNAL(itemSelectionChanged()),   this,            SLOT(selected_signals_changed()));
  QObject::connect(pm_design_button,       SIGNAL(clicked()),                this,            SLOT(pm_design_button_pressed()));
  QObject::connect(pm_add_passband_button, SIGNAL(clicked()),                this,            SLOT(pm_add_passband_button_pressed()));
  QObject::connect(pm_add_stopband_button, SIGNAL(clicked()),                this,            SLOT(pm_add_stopband_button_pressed()));
  QObject::connect(pm_param_table,         SIGNAL(cellClicked(int, int)),    this,            SLOT(pm_tabel_cell_pressed(int, int)));
  QObject::connect(pm_predefined_cbox,     SIGNAL(currentIndexChanged(int)), this,            SLOT(pm_predefined_cbox_changed(int)));

  QObject::connect(pm_param_table->model(), SIGNAL(rowsInserted(const QModelIndex &, int, int)), this, SLOT(pm_paramtable_rowcount_changed()));
  QObject::connect(pm_param_table->model(), SIGNAL(rowsRemoved(const QModelIndex &, int, int)),  this, SLOT(pm_paramtable_rowcount_changed()));

  firfilterdialog->exec();
}


UI_FIRFilterDialog::~UI_FIRFilterDialog()
{
  free(z);
  free(deg);
  free(taps);
  free(textbuf);
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
  int i, len=0, err, max_len=4;

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

    if(max_len < (int)strlen(str))
    {
      max_len = strlen(str);
    }

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

  textEdit->setMinimumWidth(max_len * 8 * mainwindow->w_scaling);

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
  db_min_i = (((int)(db_min / 10)) * 10) + 10;

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
    s = selectedlist.at(i)->data(Qt::UserRole).toInt();

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

  n = list->selectedItems().size();

  snprintf(str, 128, "Selected signals: %i", n);

  listlabel->setText(str);
}


int UI_FIRFilterDialog::pm_add_table_row(void)
{
  int i;

  QDoubleSpinBox *tmp_box;

  QLabel *tmp_label;

  i = pm_param_table->rowCount();

  if(i >= 32)  return -1;

  textEdit->clear();

  pm_param_table->setRowCount(i + 1);

  tmp_box = new QDoubleSpinBox;
  tmp_box->setRange(0, 1e6);
  tmp_box->setSuffix(" Hz");
  pm_param_table->setCellWidget(i, 0, tmp_box);

  tmp_box = new QDoubleSpinBox;
  tmp_box->setRange(0, 1e6);
  tmp_box->setSuffix(" Hz");
  pm_param_table->setCellWidget(i, 1, tmp_box);

  tmp_box = new QDoubleSpinBox;
  tmp_box->setRange(0, 100);
  pm_param_table->setCellWidget(i, 2, tmp_box);

  tmp_box = new QDoubleSpinBox;
  tmp_box->setRange(-100, 10);
  tmp_box->setSuffix(" dB");
  pm_param_table->setCellWidget(i, 3, tmp_box);

  tmp_label = new QLabel;
  tmp_label->setText("Delete");
  pm_param_table->setCellWidget(i, 4, tmp_label);

  pm_param_table->resizeColumnsToContents();

  return i;
}


void UI_FIRFilterDialog::pm_predefined_cbox_changed(int idx)
{
  double sf = 2000;

  if(!idx)  return;

  pm_param_table->setRowCount(0);

  if(mainwindow->signalcomps)
  {
    sf = mainwindow->signalcomp[0]->edfhdr->edfparam[0].sf_f;
  }

  pm_sf_spinbox->setValue(sf);

  if(idx == 1)  /* lowpass */
  {
    pm_add_passband_button_pressed();
    pm_add_stopband_button_pressed();

    ((QDoubleSpinBox *)(pm_param_table->cellWidget(0, 1)))->setValue(0.2 * sf);
    ((QDoubleSpinBox *)(pm_param_table->cellWidget(1, 0)))->setValue(0.25 * sf);
    ((QDoubleSpinBox *)(pm_param_table->cellWidget(1, 1)))->setValue(0.5 * sf);
  }

  if(idx == 2)  /* highpass */
  {
    pm_add_stopband_button_pressed();
    pm_add_passband_button_pressed();

    ((QDoubleSpinBox *)(pm_param_table->cellWidget(0, 1)))->setValue(0.2 * sf);
    ((QDoubleSpinBox *)(pm_param_table->cellWidget(1, 0)))->setValue(0.25 * sf);
    ((QDoubleSpinBox *)(pm_param_table->cellWidget(1, 1)))->setValue(0.5 * sf);
  }

  if(idx == 3)  /* bandpass */
  {
    pm_add_stopband_button_pressed();
    pm_add_passband_button_pressed();
    pm_add_stopband_button_pressed();

    ((QDoubleSpinBox *)(pm_param_table->cellWidget(0, 1)))->setValue(0.1 * sf);
    ((QDoubleSpinBox *)(pm_param_table->cellWidget(1, 0)))->setValue(0.15 * sf);
    ((QDoubleSpinBox *)(pm_param_table->cellWidget(1, 1)))->setValue(0.25 * sf);
    ((QDoubleSpinBox *)(pm_param_table->cellWidget(2, 0)))->setValue(0.3 * sf);
    ((QDoubleSpinBox *)(pm_param_table->cellWidget(2, 1)))->setValue(0.5 * sf);
  }

  if(idx == 4)  /* bandstop */
  {
    pm_add_passband_button_pressed();
    pm_add_stopband_button_pressed();
    pm_add_passband_button_pressed();

    ((QDoubleSpinBox *)(pm_param_table->cellWidget(0, 1)))->setValue(0.1 * sf);
    ((QDoubleSpinBox *)(pm_param_table->cellWidget(1, 0)))->setValue(0.15 * sf);
    ((QDoubleSpinBox *)(pm_param_table->cellWidget(1, 1)))->setValue(0.25 * sf);
    ((QDoubleSpinBox *)(pm_param_table->cellWidget(2, 0)))->setValue(0.3 * sf);
    ((QDoubleSpinBox *)(pm_param_table->cellWidget(2, 1)))->setValue(0.5 * sf);
  }

  pm_predefined_cbox->setCurrentIndex(0);
}


void UI_FIRFilterDialog::pm_add_passband_button_pressed()
{
  int i = pm_add_table_row();
  if(i < 0)  return;

  ((QDoubleSpinBox *)(pm_param_table->cellWidget(i, 2)))->setValue(1);
  ((QDoubleSpinBox *)(pm_param_table->cellWidget(i, 3)))->setValue(1);
}


void UI_FIRFilterDialog::pm_add_stopband_button_pressed()
{
  int i = pm_add_table_row();
  if(i < 0)  return;

  ((QDoubleSpinBox *)(pm_param_table->cellWidget(i, 2)))->setValue(0);
  ((QDoubleSpinBox *)(pm_param_table->cellWidget(i, 3)))->setValue(-40);
}


void UI_FIRFilterDialog::pm_tabel_cell_pressed(int row, int column)
{
  if(column == 4)
  {
    pm_param_table->removeRow(row);
  }
}


void UI_FIRFilterDialog::pm_paramtable_rowcount_changed()
{
  if(pm_param_table->rowCount() < 2)
  {
    pm_design_button->setEnabled(false);
  }
  else
  {
    pm_design_button->setEnabled(true);
  }
}


void UI_FIRFilterDialog::pm_design_button_pressed()
{
  int i,
      err=0,
      idx=0,
      buf_sz=10000;

  char *str=NULL,
       txt_str[512]={""};

  QEventLoop evlp;

  if(pm_param_table->rowCount() < 2)  return;

  textEdit->clear();

  str = (char *)malloc(buf_sz);
  if(str == NULL)
  {
    return;
  }

  thrd = new UI_FIRFilterRZ_thread();

  memset(&thrd->rz_params, 0, sizeof(struct UI_FIRFilterRZ_thread::rz_params_struct));

  thrd->rz_params.sf = 2000;

  thrd->rz_params.band_num = pm_param_table->rowCount();

  if(thrd->rz_params.band_num > 32)  thrd->rz_params.band_num = 32;

  thrd->rz_params.sf = pm_sf_spinbox->value();

  for(i=0, err=0; i<thrd->rz_params.band_num; i++)
  {
    thrd->rz_params.start_hz[i] = ((QDoubleSpinBox *)(pm_param_table->cellWidget(i, 0)))->value();
    thrd->rz_params.stop_hz[i]  = ((QDoubleSpinBox *)(pm_param_table->cellWidget(i, 1)))->value();
    thrd->rz_params.gain[i]     = ((QDoubleSpinBox *)(pm_param_table->cellWidget(i, 2)))->value();
    thrd->rz_params.ripple[i]   = ((QDoubleSpinBox *)(pm_param_table->cellWidget(i, 3)))->value();

    if((!dblcmp(thrd->rz_params.gain[i], 0)) && (thrd->rz_params.ripple[i] >= 0))
    {
      err = 1;

      snprintf(txt_str, 512, "Error in parameters for band %i: if \"Gain\" is 0 (stopband),\"Ripple\" must be lower than 0", i + 1);
      QMessageBox messagewindow(QMessageBox::Critical, "Error", txt_str);
      messagewindow.exec();

      break;
    }

    if(dblcmp(thrd->rz_params.gain[i], 0) && (thrd->rz_params.ripple[i] <= 0))
    {
      err = 1;

      snprintf(txt_str, 512, "Error in parameters for band %i: if \"Gain\" is > 0 (passband),\"Ripple\" must be greater than 0", i + 1);
      QMessageBox messagewindow(QMessageBox::Critical, "Error", txt_str);
      messagewindow.exec();

      break;
    }

    if(thrd->rz_params.stop_hz[i] <= thrd->rz_params.start_hz[i])
    {
      err = 1;

      snprintf(txt_str, 512, "Error in parameters for band %i: frequency \"To\" must be higher than frequency \"From\"", i + 1);
      QMessageBox messagewindow(QMessageBox::Critical, "Error", txt_str);
      messagewindow.exec();

      break;
    }

    if((thrd->rz_params.start_hz[i] / thrd->rz_params.stop_hz[i]) > 0.95)
    {
      err = 1;

      snprintf(txt_str, 512, "Error in parameters for band %i: freq. \"From\" divided by freq. \"To\" must be <= 0.95", i + 1);
      QMessageBox messagewindow(QMessageBox::Critical, "Error", txt_str);
      messagewindow.exec();

      break;
    }

    if(i)
    {
      if(thrd->rz_params.start_hz[i] <= thrd->rz_params.stop_hz[i-1])
      {
        err = 1;

        snprintf(txt_str, 512, "Error in parameters for band %i: frequency \"From\" must be higher than frequency \"To\" of band %i", i + 1, i);
        QMessageBox messagewindow(QMessageBox::Critical, "Error", txt_str);
        messagewindow.exec();

        break;
      }

      if((thrd->rz_params.stop_hz[i-1] / thrd->rz_params.start_hz[i]) > 0.95)
      {
        err = 1;

        snprintf(txt_str, 512, "Error in parameters for band %i: freq. \"To\" of band %i divided by freq. \"From\" of band %i must be <= 0.95", i + 1, i, i + 1);
        QMessageBox messagewindow(QMessageBox::Critical, "Error", txt_str);
        messagewindow.exec();

        break;
      }
    }
  }

  if(thrd->rz_params.stop_hz[thrd->rz_params.band_num - 1] > (thrd->rz_params.sf / 2))
  {
    err = 1;

    snprintf(txt_str, 512, "Error in parameters for band %i: freq \"To\" must be <= Fs/2", thrd->rz_params.band_num);
    QMessageBox messagewindow(QMessageBox::Critical, "Error", txt_str);
    messagewindow.exec();
  }

  if(err)
  {
    free(str);
    delete thrd;
    return;
  }

  firfilterdialog->setEnabled(false);

  QApplication::setOverrideCursor(Qt::WaitCursor);

  thrd->start();

  QObject::connect(thrd, SIGNAL(finished()), &evlp, SLOT(quit()));
  evlp.exec();

  if((thrd->rz_params.tap_num < 2) || (thrd->rz_params.f_taps == NULL))
  {
    firfilterdialog->setEnabled(true);

    QApplication::restoreOverrideCursor();

    QMessageBox::critical(mainwindow, "Error", "Cannot design filter, try changing parameters");
  }
  else
  {
    for(i=0, idx=0; i<thrd->rz_params.tap_num; i++)
    {
      idx += snprintf(str + idx, 128, "%.20f\n", thrd->rz_params.f_taps[i]);

      if(idx >= (buf_sz - 128))
      {
        buf_sz += 10000;
        str = (char *)realloc(str, buf_sz);
        if(str==NULL)
        {
          buf_sz = 0;
          break;
        }
      }
    }

    if(str)
    {
      textEdit->appendPlainText(str);
    }

    firfilterdialog->setEnabled(true);

    QApplication::restoreOverrideCursor();
  }

  delete thrd;

  free(str);
}


UI_FIRFilterRZ_thread::UI_FIRFilterRZ_thread(void)
{
  memset(&rz_params, 0, sizeof(struct rz_params_struct));

  rz_params.sf = 2000;
}


UI_FIRFilterRZ_thread::~UI_FIRFilterRZ_thread()
{
  free(rz_params.f_taps);
}


void UI_FIRFilterRZ_thread::run()
{
  rz_params.tap_num = remez(rz_params.sf,
                            rz_params.band_num,
                            rz_params.start_hz,
                            rz_params.stop_hz,
                            rz_params.gain,
                            rz_params.ripple,
                            &rz_params.f_taps);
}















