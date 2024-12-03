#include "mainwindow.h"

void MainWindow::changelog()
{
    QMessageBox msgBox;
    msgBox.setText("Hauken changelog");
    QString txt;
    QTextStream ts(&txt);
    ts << "<table>"
       << "<tr><td>2.44</td><td>OAuth authentication and file upload added. IQ plot now 'squelched' to locate signal level</td></tr>"
       << "<tr><td>2.43</td><td>IQ high res FFT plot added. Added millisecond save option to 1809 file</td></tr>"
       << "<tr><td>2.42</td><td>Datastream rebuild, various reconnect issues fixed</td></tr>"
       << "<tr><td>2.41</td><td>Jammertest version, unreleased</td></tr>"
       << "<tr><td>2.40</td><td>Camera support readded</td></tr>"
       << "<tr><td>2.39</td><td>AI category added (other). Various bugfixes</td></tr>"
       << "<tr><td>2.38</td><td>Added GNSS data display in separate window</td></tr>"
       << "<tr><td>2.37</td><td>Instrument list collected from external server if available</td></tr>"
       << "<tr><td>2.36</td><td>MQTT data can be added to incident log</td></tr>"
       << "<tr><td>2.35</td><td>Watchdog function improved. Spectrum mask bugfix. Foreign letters bugfix</td></tr>"
       << "<tr><td>2.34</td><td>AI classification and filtered notification function</td></tr>"
       << "<tr><td>2.33</td><td>Added spectrum mask overlay function (right click spectrum/\"bandplan.csv\")</td></tr>"
       << "<tr><td>2.32</td><td>Support for GNSS NMEA and binary (uBlox) data over TCP/IP</td></tr>"
       << "<tr><td>2.31</td><td>Audio and visual indicators of incidents added. Various minor bugfixes</td></tr>"
       << "<tr><td>2.30</td><td>Bugfix: Trig frequency selection failing on selection outside window. Debugging info removed</td></tr>"
       << "<tr><td>2.29</td><td>Qt library version upgrade to 6.6.0. Rebuilt code and connected libraries</td></tr>"
       << "<tr><td>2.28</td><td>Position link to google maps added to notifications email. Minor bugfixes</td></tr>"
       << "<tr><td>2.27</td><td>Minor bugfixes</td></tr>"
       << "<tr><td>2.26</td><td>Antenna names readout and editing added (EM200 and USRP specific)</td></tr>"
       << "<tr><td>2.25</td><td>Added MQTT and webswitch data report options</td></tr>"
       << "<tr><td>2.24</td><td>Added geographic blocking from KML polygon file</td></tr>"
       << "<tr><td>2.23</td><td>Periodic http report (x-www-form-urlencoded) added</td></tr>"
       << "<tr><td>2.22</td><td>Microsoft Graph email notification</td></tr>"
       << "<tr><td>2.21</td><td>EM200 GNSS suppported, added instrumentGNSS monitoring/recording</td></tr>"
       << "<tr><td>2.20</td><td>Added Arduino watchdog</td></tr>"
       << "<tr><td>2.19</td><td>ESMB UDP bugfix, other minor fixes</td></tr>"
       << "<tr><td>2.18</td><td>Average calculation halted while ongoing incident</td></tr>"
       << "<tr><td>2.17</td><td>Auto recording option added, for continous recording on startup</td></tr>"
       << "<tr><td>2.16</td><td>Added option for variable trace average calculation</td></tr>"
       << "<tr><td>2.15</td><td>Added elementary support for Arduino IO via serial (for relay control, temperature sensor, so on)</td></tr>"
       << "<tr><td>2.14</td><td>Wait for network available if set to connect on startup</td></tr>"
       << "<tr><td>2.13</td><td>InstrGNSS status window. Silly auto reconnect bugfix</td></tr>"
       << "<tr><td>2.10</td><td>Added camera options. UDP lost datagram bugfix</td></tr>"
       << "<tr><td>2.9</td><td>File upload and email notification auto resend on failure. Manual trig added. Minor trace display bugfixes</td></tr>"
       << "<tr><td>2.8</td><td>SDeF mobile position added. Minor bugfixes in waterfall code</td></tr>"
       << "<tr><td>2.7</td><td>Waterfall added with a ton of colors. Minor notifications bugfixes</td></tr>"
       << "<tr><td>2.6</td><td>Email notifications, inline pictures in email</td></tr>"
       << "<tr><td>2.5</td><td>Dual GNSS support added</td></tr>"
       << "<tr><td>2.4</td><td>Auto upload to Casper added, minor bugfixes</td></tr>"
       << "<tr><td>2.3</td><td>Instrument support: ESMB, EM100, EM200, EB500, USRP/Tracy</td></tr>"
       << "<tr><td>2.2</td><td>Basic file saving enabled</td></tr>"
       << "<tr><td>2.1</td><td>Plot and buffer functions working</td></tr>"
       << "<tr><td>2.0.b</td>" << "<td>Initial commit, basic instrument control</td></tr>";
    ts << "</table>";
    msgBox.setInformativeText(txt);

    msgBox.exec();
}

