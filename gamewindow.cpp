#include "gamewindow.h"

#include <QtGui/QGuiApplication>
#include <QtGui/QMatrix4x4>
#include <QtGui/QOpenGLShaderProgram>
#include <QtGui/QScreen>

#include <QtCore/qmath.h>
#include <QMouseEvent>
#include <QKeyEvent>
#include <time.h>
#include <sys/time.h>
#include <iostream>

#include <QtCore>
#include <QtGui>

#include <QTimer>

using namespace std;

static bool animate = true; //permet de savoir si on anime les terrain des différentes fenêtre GameWindow en les faisant tourner selon l'axe y

/**
 * @brief GameWindow::GameWindow, constructeur de la classe GameWindow.
 * @param refresh_rate, taux de rafraîchissement de la fenêtre
 * @param c, paramètre facultatif qui permet d'avoir une caméra partagée par plusieurs fenêtres
 */
GameWindow::GameWindow(int refresh_rate, Camera* c) : carte(1), m_refresh_rate(refresh_rate), speed(0.5f)
{
    srand(time(NULL));

    if(c != 0){
        share_cam = true;
        m_camera = c;
    }else{
        share_cam = false;
        m_camera = new Camera();
    }

    season = "NONE";

    m_timer = new QTimer(this);
    connect(m_timer,SIGNAL(timeout()),this, SLOT(renderNow()));

    restartTimer();

    doConnect();
}

/**
 * @brief GameWindow::~GameWindow, destructeur de la classe GameWindow.
 */
GameWindow::~GameWindow(){
    delete p;

    if(!share_cam)
        delete m_camera;

    delete m_timer;
}

/**
 * @brief GameWindow::initialize, initialise les fonctionnalités OpenGL et charge une heightmap.
 */
void GameWindow::initialize()
{
    const qreal retinaScale = devicePixelRatio();


    glViewport(0, 0, width() * retinaScale, height() * retinaScale);

    glClearColor(0.0, 0.0, 0.0, 0.0);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-1.0, 1.0, -1.0, 1.0, -100.0, 100.0);

    loadMap(":/heightmap-2.png");
    createParticles();
}

/**
 * @brief GameWindow::loadMap, permet de charger une heightmap.
 * @param localPath, chemin vers la heightmap
 */
void GameWindow::loadMap(QString localPath)
{

    if (QFile::exists(localPath)) {
        m_image = QImage(localPath);
    }

    uint id = 0;
    p = new point[m_image.width() * m_image.height()];
    QRgb pixel;
    for(int i = 0; i < m_image.width(); i++)
    {
        for(int j = 0; j < m_image.height(); j++)
        {

            pixel = m_image.pixel(i,j);

            id = i*m_image.width() +j;

            p[id].x = (float)i/(m_image.width()) - ((float)m_image.width()/2.0)/m_image.width();
            p[id].y = (float)j/(m_image.height()) - ((float)m_image.height()/2.0)/m_image.height();
            p[id].z = 0.001f * (float)(qRed(pixel));
        }
    }

    min_x = p[0].x;
    min_y = p[0].y;

    max_x = p[m_image.width() * m_image.height() - 1].x;
    max_y = p[m_image.width() * m_image.height() - 1].y;
}

/**
 * @brief GameWindow::render, fonction de rendu OpenGL contenant plusieurs type de rendu.
 */
void GameWindow::render()
{

    glClear(GL_COLOR_BUFFER_BIT);

    glLoadIdentity();
    glScalef(m_camera->ss,m_camera->ss,m_camera->ss);
    glRotatef(m_camera->rotX,1.0f,0.0f,0.0f);
    glRotatef(m_camera->rotY,0.0f,0.0f,1.0f);

    switch(m_camera->etat)
    {
    case 0:
        displayPoints();
        break;
    case 1:
        displayLines();
        break;
    case 2:
        displayTriangles();
        break;
    case 3:
        displayTrianglesC();
        break;
    case 4:
        displayTrianglesTexture();
        break;
    case 5:

        displayTrianglesTexture();
        displayLines();
        break;
    default:
        displayPoints();
        break;
    }

    displayParticles();

    if(animate){
        animWindow();
    }

    ++m_frame;
}

/**
 * @brief GameWindow::event, fonction permettant la gestion d'évenements.
 * @param event, QEvent
 * @return
 */
bool GameWindow::event(QEvent *event)
{
    switch (event->type())
    {
    case QEvent::UpdateRequest:

        renderNow();
        return true;
    default:
        return QWindow::event(event);
    }
}

/**
 * @brief GameWindow::keyPressEvent, permet de gérer les évenements de type clavier.
 * @param event, évenement de type clavier
 */
void GameWindow::keyPressEvent(QKeyEvent *event)
{
    switch(event->key())
    {
    case 'Z':
        m_camera->ss += 0.10f;
        break;
    case 'S':
        m_camera->ss -= 0.10f;
        break;
    case 'A':
        m_camera->rotX += 1.0f;
        break;
    case 'E':
        m_camera->rotX -= 1.0f;
        break;
    case 'Q':
        m_camera->rotY += 1.0f;
        break;
    case 'D':
        m_camera->rotY -= 1.0f;
        break;
    case 'W':
        m_camera->etat ++;
        if(m_camera->etat > 5)
            m_camera->etat = 0;
        break;
    case 'C':
        animate = !animate;
        break;
    case 'P':
        m_refresh_rate *= 2;
        if(m_refresh_rate > 120)
            m_refresh_rate = 120;

        restartTimer();
        break;
    case 'M':
        m_refresh_rate /= 2;
        if(m_refresh_rate < 1)
            m_refresh_rate = 1;

        restartTimer();
        break;
    case 'X':
        carte ++;
        if(carte > 3)
            carte = 1;
        QString depth (":/heightmap-");
        depth += QString::number(carte) ;
        depth += ".png" ;

        loadMap(depth);
        break;
    }
}

/**
 * @brief GameWindow::displayPoints, fonction d'affichage du terrain avec des points.
 */
void GameWindow::displayPoints()
{
    glColor3f(1.0f, 1.0f, 1.0f);
    glBegin(GL_POINTS);

    seasonColor();

    uint id = 0;
    for(int i = 0; i < m_image.width(); i++)
    {
        for(int j = 0; j < m_image.height(); j++)
        {
            id = i*m_image.width() +j;
            glVertex3f(
                        p[id].x,
                        p[id].y,
                        p[id].z);

        }
    }
    glEnd();
}

/**
 * @brief GameWindow::displayTriangles, fonction d'affichage du terrain avec des triangles.
 */
void GameWindow::displayTriangles()
{
    seasonColor();
    glBegin(GL_TRIANGLES);
    uint id = 0;

    for(int i = 0; i < m_image.width()-1; i++)
    {
        for(int j = 0; j < m_image.height()-1; j++)
        {

            id = i*m_image.width() +j;
            glVertex3f(
                        p[id].x,
                        p[id].y,
                        p[id].z);
            id = i*m_image.width() +(j+1);
            glVertex3f(
                        p[id].x,
                        p[id].y,
                        p[id].z);
            id = (i+1)*m_image.width() +j;
            glVertex3f(
                        p[id].x,
                        p[id].y,
                        p[id].z);



            id = i*m_image.width() +(j+1);
            glVertex3f(
                        p[id].x,
                        p[id].y,
                        p[id].z);
            id = (i+1)*m_image.width() +j+1;
            glVertex3f(
                        p[id].x,
                        p[id].y,
                        p[id].z);
            id = (i+1)*m_image.width() +j;
            glVertex3f(
                        p[id].x,
                        p[id].y,
                        p[id].z);
        }
    }

    glEnd();
}

/**
 * @brief GameWindow::displayTrianglesC, fonction d'affichage du terrain avec des triangles de couleurs différentes.
 */
void GameWindow::displayTrianglesC()
{
    glColor3f(1.0f, 1.0f, 1.0f);
    glBegin(GL_TRIANGLES);
    uint id = 0;

    for(int i = 0; i < m_image.width()-1; i++)
    {
        for(int j = 0; j < m_image.height()-1; j++)
        {
            glColor3f(0.0f, 1.0f, 0.0f);
            id = i*m_image.width() +j;
            glVertex3f(
                        p[id].x,
                        p[id].y,
                        p[id].z);
            id = i*m_image.width() +(j+1);
            glVertex3f(
                        p[id].x,
                        p[id].y,
                        p[id].z);
            id = (i+1)*m_image.width() +j;
            glVertex3f(
                        p[id].x,
                        p[id].y,
                        p[id].z);


            glColor3f(1.0f, 1.0f, 1.0f);
            id = i*m_image.width() +(j+1);
            glVertex3f(
                        p[id].x,
                        p[id].y,
                        p[id].z);
            id = (i+1)*m_image.width() +j+1;
            glVertex3f(
                        p[id].x,
                        p[id].y,
                        p[id].z);
            id = (i+1)*m_image.width() +j;
            glVertex3f(
                        p[id].x,
                        p[id].y,
                        p[id].z);
        }
    }
    glEnd();
}

/**
 * @brief GameWindow::displayLines, fonction d'affichage du terrain avec des lignes.
 */
void GameWindow::displayLines()
{
    seasonColor();
    glBegin(GL_LINES);
    uint id = 0;

    for(int i = 0; i < m_image.width()-1; i++)
    {
        for(int j = 0; j < m_image.height()-1; j++)
        {

            id = i*m_image.width() +j;
            glVertex3f(
                        p[id].x,
                        p[id].y,
                        p[id].z);
            id = i*m_image.width() +(j+1);
            glVertex3f(
                        p[id].x,
                        p[id].y,
                        p[id].z);

            id = (i+1)*m_image.width() +j;
            glVertex3f(
                        p[id].x,
                        p[id].y,
                        p[id].z);
            id = i*m_image.width() +j;
            glVertex3f(
                        p[id].x,
                        p[id].y,
                        p[id].z);

            id = (i+1)*m_image.width() +j;
            glVertex3f(
                        p[id].x,
                        p[id].y,
                        p[id].z);
            id = i*m_image.width() +(j+1);
            glVertex3f(
                        p[id].x,
                        p[id].y,
                        p[id].z);

            id = i*m_image.width() +(j+1);
            glVertex3f(
                        p[id].x,
                        p[id].y,
                        p[id].z);
            id = (i+1)*m_image.width() +j+1;
            glVertex3f(
                        p[id].x,
                        p[id].y,
                        p[id].z);

            id = (i+1)*m_image.width() +j+1;
            glVertex3f(
                        p[id].x,
                        p[id].y,
                        p[id].z);

            id = (i+1)*m_image.width() +(j);
            glVertex3f(
                        p[id].x,
                        p[id].y,
                        p[id].z);
        }
    }

    glEnd();
}

/**
 * @brief GameWindow::displayTrianglesTexture, fonction d'affichage du terrain avec des triangles texturés en fonction de la hauteur des vertices.
 */
void GameWindow::displayTrianglesTexture()
{
    glColor3f(1.0f, 1.0f, 1.0f);
    glBegin(GL_TRIANGLES);
    uint id = 0;

    for(int i = 0; i < m_image.width()-1; i++)
    {
        for(int j = 0; j < m_image.height()-1; j++)
        {

            id = i*m_image.width() +j;
            displayColor(p[id].z);
            glVertex3f(
                        p[id].x,
                        p[id].y,
                        p[id].z);
            id = i*m_image.width() +(j+1);
            displayColor(p[id].z);
            glVertex3f(
                        p[id].x,
                        p[id].y,
                        p[id].z);
            id = (i+1)*m_image.width() +j;
            displayColor(p[id].z);
            glVertex3f(
                        p[id].x,
                        p[id].y,
                        p[id].z);



            id = i*m_image.width() +(j+1);
            displayColor(p[id].z);
            glVertex3f(
                        p[id].x,
                        p[id].y,
                        p[id].z);
            id = (i+1)*m_image.width() +j+1;
            displayColor(p[id].z);
            glVertex3f(
                        p[id].x,
                        p[id].y,
                        p[id].z);
            id = (i+1)*m_image.width() +j;
            displayColor(p[id].z);
            glVertex3f(
                        p[id].x,
                        p[id].y,
                        p[id].z);
        }
    }
    glEnd();
}

/**
 * @brief GameWindow::displayColor, affiche une couleur sur les vertices en fonction de l'altitude de ceux-ci.
 * @param alt, altitude en z
 */
void GameWindow::displayColor(float alt)
{
    if (alt > 0.2)
    {
        glColor3f(1.0f, 1.0f, 1.0f);
    }
    else     if (alt > 0.1)
    {
        glColor3f(alt, 1.0f, 1.0f);
    }
    else     if (alt > 0.05f)
    {
        glColor3f(1.0f, alt, alt);
    }
    else
    {
        glColor3f(0.0f, 0.0f, 1.0f);
    }

}

/**
 * @brief GameWindow::animWindow, fonction permettant la rotation automatique du terrain selon l'axe y.
 */
void GameWindow::animWindow(){
    m_camera->rotY += speed;
}

/**
 * @brief GameWindow::restartTimer, permet de relancer le timer lorsque celui-ci change.
 */
void GameWindow::restartTimer(){
    m_timer->stop();
    m_timer->start(1000.f / m_refresh_rate);

    updateTitle();
}

void GameWindow::updateTitle(){
    QString fps = QString::number(m_refresh_rate);
    QString title = fps + "FPS - " + season;
    setTitle(title);
}

void GameWindow::seasonColor(){
    if(season == "PRINTEMPS"){
        glColor3f(0.f, 0.7f, 0.2f);
    }
    if(season == "ETE"){
        glColor3f(1.f, 1.f, 0.f);
    }
    else if(season == "AUTOMNE"){
        glColor3f(0.86f, 0.4f, 0.f);
    }
    else if(season == "HIVER"){
        glColor3f(0.8f, 0.8f, 0.8f);
    }
}

void GameWindow::createParticles(){
    tab_particles = new particles[MAX_PARTICLES];
    for(unsigned int i = 0 ; i < MAX_PARTICLES ; i++){
        tab_particles[i] = newParticle();
    }
}

particles GameWindow::newParticle(){
    particles part;
    int rand_x, rand_y, id;

    rand_x = rand() % m_image.width();
    rand_y = rand() % m_image.height();
    id = rand_y * m_image.width() + rand_x;

    part.x = p[id].x;
    part.y = p[id].y;
    part.z = (rand() % 5 + 3) / 10.f;
    part.min_z = p[id].z;
    part.falling_speed = (rand() % 10 + 1) / 1000.f;

    return part;
}


void GameWindow::displayParticles(){
    if(season == "AUTOMNE"){
        glColor3f(0.0f, 0.0f, 1.0f);
    }else if(season == "HIVER"){
        glColor3f(1.0f, 1.0f, 1.0f);
    }else{
        return;
    }

    glBegin(GL_POINTS);
    for(unsigned int i = 0 ; i < MAX_PARTICLES ; i++){
        glVertex3f(tab_particles[i].x, tab_particles[i].y, tab_particles[i].z);

        tab_particles[i].z -= tab_particles[i].falling_speed;

        if(tab_particles[i].z < tab_particles[i].min_z){
            tab_particles[i] = newParticle();
        }
    }
    glEnd();
}

void GameWindow::doConnect(){
    socket = new QTcpSocket(this);

    connect(socket, SIGNAL(connected()),this, SLOT(connected()));
    connect(socket, SIGNAL(disconnected()),this, SLOT(disconnected()));
    connect(socket, SIGNAL(bytesWritten(qint64)),this, SLOT(bytesWritten(qint64)));
    connect(socket, SIGNAL(readyRead()),this, SLOT(readyRead()));

    qDebug() << QString::number(m_refresh_rate) << "FPS window : connecting...";

    // this is not blocking call
    socket->connectToHost("127.0.0.1", 9999);

    // we need to wait...
    if(!socket->waitForConnected(5000))
    {
        qDebug() << "Error: " << socket->errorString();
    }
}

/** SLOTS **/
void GameWindow::connected()
{
    //qDebug() << QString::number(m_refresh_rate) << "FPS window : connected...";
}

void GameWindow::disconnected()
{
    //qDebug() << QString::number(m_refresh_rate) << "FPS window : disconnected...";
}

void GameWindow::bytesWritten(qint64 bytes)
{
    //qDebug() << QString::number(m_refresh_rate) << "FPS window : " << bytes << " bytes written...";
}

void GameWindow::readyRead()
{
    //qDebug() << QString::number(m_refresh_rate) << "FPS window : reading...";

    season = QString(socket->readAll());

    // read the data from the socket
    //qDebug() << QString::number(m_refresh_rate) << "FPS window : " << season;

    updateTitle();
}
