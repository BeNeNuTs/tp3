#ifndef GAMEWINDOW_H
#define GAMEWINDOW_H

#include "openglwindow.h"
#include "camera.h"

#include <QTcpSocket>
#include <QAbstractSocket>

struct point
{
    float x, y ,z;
};

struct particles
{
    float x, y ,z;
    float falling_speed;
    float min_z;
};


class GameWindow : public OpenGLWindow
{
signals:

public slots:
    void connected();
    void disconnected();
    void readyRead();

public:
    GameWindow(int refresh_rate, Camera* c = 0);
    ~GameWindow();

    void initialize();
    void render();
    bool event(QEvent *event);

    void keyPressEvent(QKeyEvent *event);

    void displayTriangles();
    void displayLines();
    void displayTrianglesC();
    void displayPoints();
    void displayTrianglesTexture();

    void displayColor(float);

    void loadMap(QString localPath);

    void animWindow();
    void restartTimer();

    void updateTitle();
    void seasonColor();

    void createParticles();
    particles newParticle();
    void displayParticles();

    void doConnect();

public:
    static const unsigned int MAX_PARTICLES = 1000;

private:

    int m_frame;
    QImage m_image;
    point *p;


    int carte;
    int m_refresh_rate; 		// Taux de rafraîchissement de la fenêtre.

    bool share_cam;				// Permet de savoir si la caméra est partagée par plusieurs instances.
    Camera* m_camera;			// Pointeur vers la caméra de la fenêtre.

    QTimer* m_timer;			// Timer permettant un rafraîchissement à temps fixe de la fenêtre.

    float speed;				// Vitesse de rotation du terrain selon l'axe y quand animate = true.

    QString season;             // Contient le nom de la saison courante dans la fenêtre
    QTcpSocket *socket;         // QTcpSocket permettant de se connecter au serveur

    particles* tab_particles;   // Tableau contenant les particules de pluie/neige
};


#endif // GAMEWINDOW_H
