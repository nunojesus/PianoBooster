/*!
    @file           QtWindow.cpp

    @brief          xxx.

    @author         L. J. Barman

    Copyright (c)   2008, L. J. Barman, all rights reserved

    This file is part of the PianoBooster application

    PianoBooster is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    PianoBooster is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with PianoBooster.  If not, see <http://www.gnu.org/licenses/>.

*/

#include "GlView.h"
#include "QtWindow.h"
#include "ReleaseNote.txt"

Window::Window()
{
//    QGLFormat fmt; //fixme
//    fmt.setSwapInterval(1);
//    QGLFormat::setDefaultFormat(fmt);

    QCoreApplication::setOrganizationName("PianoBooster");
    QCoreApplication::setOrganizationDomain("pianobooster.sourceforge.net/");
    QCoreApplication::setApplicationName("Piano Booster");
    m_settings = new QSettings(QSettings::IniFormat, QSettings::UserScope, "PianoBooster", "Piano Booster");

    setWindowTitle(tr("Piano Booster"));

    m_glWidget = new CGLView(this);
    m_song = m_glWidget->getSongObject();
    m_score = m_glWidget->getScoreObject();


    QHBoxLayout *mainLayout = new QHBoxLayout;
    QVBoxLayout *columnLayout = new QVBoxLayout;

    m_sidePanel = new GuiSidePanel(this, m_settings);
    m_topBar = new GuiTopBar();

    decodeCommandLine();

    mainLayout->addWidget(m_sidePanel);
    columnLayout->addWidget(m_topBar);
    columnLayout->addWidget(m_glWidget);
    mainLayout->addLayout(columnLayout);

    m_song->setScore(m_score);
    m_song->init();
    m_glWidget->init();

    m_sidePanel->init(m_song, m_song->getTrackList(), m_topBar);
    m_topBar->init(m_song, m_song->getTrackList());
    createActions();
    createSongControls();
    createMenus();
    //createToolBars();
    readSettings();

    QWidget *centralWin = new QWidget();
    centralWin->setLayout(mainLayout);

    setCentralWidget(centralWin);

    m_glWidget->setFocus(Qt::ActiveWindowFocusReason);

    show();


    m_song->setPianoSoundPatches(m_settings->value("Keyboard/RightSound", 1).toInt() - 1,
                                 m_settings->value("Keyboard/WrongSound", 24).toInt() - 1);

    QString midiInputName = m_settings->value("midi/input").toString();
    if (midiInputName.startsWith("None"))
        CChord::setPianoRange(PC_KEY_LOWEST_NOTE, PC_KEY_HIGHEST_NOTE);
    else
        CChord::setPianoRange(m_settings->value("Keyboard/lowestNote", 0).toInt(),
                          m_settings->value("Keyboard/highestNote", 127).toInt());

    m_song->openMidiPort(0, string(midiInputName.toAscii()));

    if (m_song->openMidiPort(1,string(m_settings->value("midi/output").toString().toAscii()))==false)
    {
        showMidiSetup();
    }
}

Window::~Window()
{
    delete m_settings;
}

void Window::decodeCommandLine()
{
    QStringList argList = QCoreApplication::arguments();
    for (int i = 0; i < argList.size(); ++i)
    {
        if (argList.at(i).startsWith("-d"))
            Cfg::debugLevel++;
        if (argList.at(i).startsWith("-s"))
            CStavePos::setStaveCentralOffset(60);
        if (argList.at(i).startsWith("-q"))
            Cfg::quickStart = true;
    }
}

void Window::createActions()
{
    m_openAct = new QAction(QIcon(":/images/open.png"), tr("&Open..."), this);
    m_openAct->setShortcut(tr("Ctrl+O"));
    m_openAct->setStatusTip(tr("Open an existing file"));
    connect(m_openAct, SIGNAL(triggered()), this, SLOT(open()));

    m_exitAct = new QAction(tr("E&xit"), this);
    m_exitAct->setShortcut(tr("Ctrl+Q"));
    m_exitAct->setStatusTip(tr("Exit the application"));
    connect(m_exitAct, SIGNAL(triggered()), this, SLOT(close()));

    m_aboutAct = new QAction(tr("&About"), this);
    m_aboutAct->setStatusTip(tr("Show the application's About box"));
    connect(m_aboutAct, SIGNAL(triggered()), this, SLOT(about()));

    m_setupMidiAct = new QAction(QIcon(":/images/open.png"), tr("&Midi Setup ..."), this);
    m_setupMidiAct->setShortcut(tr("Ctrl+S"));
    m_setupMidiAct->setStatusTip(tr("Setup the Midi input an output"));
    connect(m_setupMidiAct, SIGNAL(triggered()), this, SLOT(showMidiSetup()));

    m_setupKeyboardAct = new QAction(QIcon(":/images/open.png"), tr("&Keboard setting ..."), this);
    m_setupKeyboardAct->setShortcut(tr("Ctrl+K"));
    m_setupKeyboardAct->setStatusTip(tr("Setup the Midi input an output"));
    connect(m_setupKeyboardAct, SIGNAL(triggered()), this, SLOT(showKeyboardSetup()));
}

void Window::createMenus()
{
    m_fileMenu = menuBar()->addMenu(tr("&File"));
    m_fileMenu->addAction(m_openAct);
    m_fileMenu->addSeparator();
    m_fileMenu->addAction(m_exitAct);
    menuBar()->addSeparator();

    m_setupMenu = menuBar()->addMenu(tr("&Setup"));
    m_setupMenu->addAction(m_setupMidiAct);
    m_setupMenu->addAction(m_setupKeyboardAct);

    m_helpMenu = menuBar()->addMenu(tr("&Help"));
    m_helpMenu->addAction(m_aboutAct);
}

void Window::createToolBars()
{
    m_songToolBar = addToolBar(tr("Song"));
    m_songToolBar->addAction(m_openAct);
    m_songButton = new QPushButton("&Play", this);
    m_songButton->setCheckable(true);
    m_songToolBar->addWidget(m_songButton);
    connect(m_songButton, SIGNAL(clicked (bool)), this, SLOT(playMusic(bool)));

    m_songToolBar->addWidget(new QLabel("Speed", this));
    m_speedSpin = new QSpinBox(this);
    m_speedSpin->setMaximum(200);
    m_speedSpin->setMinimum(20);
    m_speedSpin->setSuffix(" %");
    m_speedSpin->setSingleStep(2);
    m_speedSpin->setValue(100);

    m_songToolBar->addWidget(m_speedSpin);
    connect(m_speedSpin, SIGNAL(valueChanged (int)), this, SLOT(setSpeed(int)));
}

void Window::createSongControls()
{
}

void Window::about()
{
   QMessageBox::about(this, tr("About"),
            tr(
                "<b>PainoBooster - Version " PP_VERSION "</b> <br><br>"
                "A fun way of <b>Boosting</b> your <b>Piano</b> playing skills!<br><br>"
                "<a href=\"http://pianobooster.sourceforge.net/\" ><b>http://pianobooster.sourceforge.net</b></a><br><br>"
                "Copyright(c) L. J. Barman, 2008; All rights reserved.<br><br>"
                "This program is made available "
                "under the terms of the GNU General Public License version 3 as published by "
                "the Free Software Foundation.<br><br>"
                "This program also contains RtMIDI: realtime MIDI i/o C++ classes<br>"
                "Copyright(c) 2003-2007 Gary P. Scavone"
                ));
}

void Window::open()
{
    QString currentSong = m_settings->value("CurrentSong").toString();

    QString fileName = QFileDialog::getOpenFileName(this,tr("Open Midi File"),
                            currentSong, tr("Midi Files (*.mid *.kar)"));
    if (!fileName.isEmpty())
        m_sidePanel->openSongFile(fileName);
}

void Window::readSettings()
{
    QPoint pos = m_settings->value("window/pos", QPoint(0, 0)).toPoint();
    QSize size = m_settings->value("window/size", QSize(600, 400)).toSize();
    resize(size);
    move(pos);
}

void Window::writeSettings()
{
     m_settings->setValue("window/pos", pos());
     m_settings->setValue("window/size", size());
}

void Window::closeEvent(QCloseEvent *event)
{
    if (QFile::exists(m_sidePanel->getCurrentSongFileName()))
        m_settings->setValue("CurrentSong",m_sidePanel->getCurrentSongFileName());

    writeSettings();
}


void Window::keyPressEvent ( QKeyEvent * event )
{
    if (event->text().length() == 0)
        return;

    if (event->isAutoRepeat() == true)
        return;

    int c = event->text().toAscii().at(0);
    m_song->pcKeyPress( c, true);
}

void Window::keyReleaseEvent ( QKeyEvent * event )
{
    if (event->isAutoRepeat() == true)
        return;

    if (event->text().length() == 0)
        return;

    int c = event->text().toAscii().at(0);
    m_song->pcKeyPress( c, false);
}
