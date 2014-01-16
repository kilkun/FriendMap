#include "PaintLayer.h"
#include <QMessageBox>
#include <QFileInfo>
#include <QMap>
#include <retroshare/rsdisc.h>
#include <marble/GeoDataLineString.h>

PaintLayer::PaintLayer(RsPeers *peers, const FriendMapSettings *settings)
{
    this->rsPeers = peers;
    const std::string& geoip_data_path = settings->getGeoIpDataPath();
	showingLinks = settings->getShowLinks();
    this->geoip = GeoIP_open(geoip_data_path.c_str(), GEOIP_STANDARD);
    if(geoip == NULL){
        std::cerr<<"Error: GeoLiteCity.dat not found!"<<std::endl;
        //QMessageBox::information(this, tr("FriendMap"), QString(tr("Error: GeoLiteCity.dat not found!")));
    }
	genPeerCache();

}

PaintLayer::~PaintLayer()
{
    delete geoip;
}

QStringList PaintLayer::renderPosition() const
{
    // We will paint in exactly one of the following layers.
    // The current one can be changed by pressing the '+' key
    /*
     * "NONE"
     * "STARS"
     * "BEHIND_TARGET"
     * "SURFACE"
     * "HOVERS_ABOVE_SURFACE"
     * "ATMOSPHERE"
     * "ORBIT"
     * "ALWAYS_ON_TOP"
     * "FLOAT_ITEM"
     * "USER_TOOLS"
     */
    QStringList layers = QStringList() << "ALWAYS_ON_TOP"<<"SURFACE" << "HOVERS_ABOVE_SURFACE";
    layers << "ORBIT" << "USER_TOOLS" << "STARS";
    return QStringList()<<layers.at(0);
}

class GeoPeerLoc{
public:
	QString gpg_id;
	QString ssl_id;
	QString name;
	GeoDataCoordinates coord;

};

class GeoPeer{
public:
	QString gpg_id;
	QList<GeoPeerLoc> locations;
	QMap<QString, GeoPeer> connections;
	QList<QString> connectionsList;
};

QList<GeoPeer> geoPeers;
QMap<std::string, GeoPeer> peerTable;
void PaintLayer::genPeerCache(){

	std::list<std::string> gpg_ids;
    rsPeers->getGPGAcceptedList(gpg_ids);
	gpg_ids.push_back(rsPeers->getGPGId(rsPeers->getOwnId()));

	//QList<GeoPeer> geoPeers;
	//QMap<QString, GeoPeer> peerTable;
	geoPeers.clear();
	peerTable.clear();
	//LIST GPG
	foreach(const std::string& gpg_id, gpg_ids){
		GeoPeer gFriend;
		gFriend.locations.clear();
        RsPeerDetails peer_details;
        //rsPeers->getGPGDetails(gpg_id, gpg_detail);
        rsPeers->getPeerDetails(gpg_id, peer_details);

        std::list<std::string> ssl_ids;
        rsPeers->getAssociatedSSLIds(gpg_id, ssl_ids);

		//LIST SSL
        foreach(const std::string& ssl_id, ssl_ids){
            RsPeerDetails peer_ssl_details;
            rsPeers->getPeerDetails(ssl_id, peer_ssl_details);
            if(peer_ssl_details.extAddr.compare("0.0.0.0")){
                //std::cout<<peer_ssl_details.extAddr<<"\n";
                GeoIPRecord *r = GeoIP_record_by_name(geoip, peer_ssl_details.extAddr.c_str());
				if(r){
					GeoPeerLoc sLocation;
					sLocation.coord = GeoDataCoordinates(r->longitude, r->latitude, 0.0, GeoDataCoordinates::Degree);
					sLocation.ssl_id = QString::fromStdString(ssl_id);
					sLocation.name = QString::fromUtf8(peer_ssl_details.name.c_str());
					gFriend.locations.push_back(sLocation);
				}

            }

        }
		if(gFriend.locations.length()>0){
			gFriend.gpg_id = QString::fromStdString(gpg_id);
			geoPeers.push_back(gFriend);
			peerTable.insert(gpg_id,gFriend);

			std::list<std::string> friendList;
			rsDisc->getDiscGPGFriends(gpg_id, friendList);
			//gFriend->connections.clear();
			//gFriend->connectionsList.clear();

			foreach(const std::string& gpg_id, friendList){
				QString q_id = QString::fromStdString(gpg_id);
				//GeoPeer gp = peerTable[q_id];
				//geoPeer->connections.insert(q_id,gp);
				gFriend.connectionsList.push_back(q_id);
			}
		}

    }

/*
	QList<GeoPeer>::iterator geoPeer;
	for (geoPeer = geoPeers.begin(); geoPeer != geoPeers.end(); ++geoPeer){
    //    cout << *i << endl;

	//foreach(GeoPeer geoPeer, geoPeers){
		std::list<std::string> friendList;
		rsDisc->getDiscGPGFriends(geoPeer->gpg_id.toStdString(), friendList);
		geoPeer->connections.clear();
		geoPeer->connectionsList.clear();
		foreach(const std::string& gpg_id, friendList){
			QString q_id = QString::fromStdString(gpg_id);
			GeoPeer gp = peerTable[q_id];
			geoPeer->connections.insert(q_id,gp);
			geoPeer->connectionsList.push_back(gp);
		}

    }*/

}

bool PaintLayer::render( GeoPainter *painter, ViewportParams *viewport,
                         const QString& renderPos, GeoSceneLayer * layer )
{

	srand(42);

	foreach(const GeoPeer& geoPeerc, geoPeers){
		GeoPeer geoPeer = geoPeerc;

		foreach(const GeoPeerLoc& geoPeerLoc, geoPeer.locations){

			float rr = ((double) rand() / (RAND_MAX));
			GeoDataCoordinates coord = geoPeerLoc.coord;//(coordFar.longitude(GeoDataCoordinates::Degree), coord.latitude(GeoDataCoordinates::Degree), 0.0, GeoDataCoordinates::Degree);

			if(rsPeers->isOnline(geoPeerLoc.ssl_id.toStdString()))
				painter->setPen(Qt::green);
			else
				painter->setPen(Qt::red);
			painter->drawEllipse(geoPeerLoc.coord, 2.5f+5.f*rr, 2.5f+5.f*rr);
			/*GeoDataLineString al;
			al.append(coord);
			al.append(geoPeerLoc.coord);
			painter->drawPolyline(al);*/
			painter->drawEllipse(coord, 10, 10);
			painter->setPen(Qt::black);
			painter->drawText(coord, geoPeerLoc.name);


			if(showingLinks){
				std::list<std::string> friendList;
				rsDisc->getDiscGPGFriends(geoPeer.gpg_id.toStdString(), friendList);
				std::cerr << friendList.size();
				foreach(const std::string& gpg_id, friendList){
					//foreach(const GeoPeer& connectionPeer, geoPeer.connectionsList){
					//std::cerr << connectionPeer.gpg_id.toStdString();
					GeoPeer other = peerTable[gpg_id];
					if (other.locations.length()>0){
						GeoPeerLoc oloc = other.locations.first();
						GeoDataLineString conLine;
						conLine.append(oloc.coord);
						conLine.append(geoPeerLoc.coord);
						painter->drawPolyline(conLine);
					} else std::cerr << "error, missing all locations for a peer\n";
				}
			}

		}
	}

    return true;
}
/*
bool PaintLayer::render( GeoPainter *painter, ViewportParams *viewport,
                         const QString& renderPos, GeoSceneLayer * layer )
{
	genPeerCache();
    std::list<std::string> gpg_ids;
    rsPeers->getGPGAcceptedList(gpg_ids);

	srand(42);
    foreach(const std::string& gpg_id, gpg_ids){
        RsPeerDetails peer_details;
        //rsPeers->getGPGDetails(gpg_id, gpg_detail);
        rsPeers->getPeerDetails(gpg_id, peer_details);

        std::list<std::string> ssl_ids;
        rsPeers->getAssociatedSSLIds(gpg_id, ssl_ids);


        foreach(const std::string& ssl_id, ssl_ids){
            RsPeerDetails peer_ssl_details;
            rsPeers->getPeerDetails(ssl_id, peer_ssl_details);
            if(peer_ssl_details.extAddr.compare("0.0.0.0")){
                //std::cout<<peer_ssl_details.extAddr<<"\n";
                GeoIPRecord *r = GeoIP_record_by_name(geoip, peer_ssl_details.extAddr.c_str());
				if(r){
					float xr = ((double) rand() / (RAND_MAX))-0.5f;
					float yr = ((double) rand() / (RAND_MAX))-0.5f;
					GeoDataCoordinates coordFar(r->longitude, r->latitude, 0.0, GeoDataCoordinates::Degree);
					GeoDataCoordinates coord(r->longitude+xr*0.3f, r->latitude+yr*0.3f, 0.0, GeoDataCoordinates::Degree);

					GeoDataLineString al;
					al.append(coord);
					al.append(coordFar);
					delete r;
					if(rsPeers->isOnline(ssl_id))
						painter->setPen(Qt::green);
					else
						painter->setPen(Qt::red);
					float rr = ((double) rand() / (RAND_MAX));
					painter->drawEllipse(coordFar, 2.5f+5.f*rr, 2.5f+5.f*rr);
					painter->drawPolyline(al);
					painter->drawEllipse(coord, 10, 10);
					painter->setPen(Qt::black);
					painter->drawText(coord, QString::fromStdString(peer_ssl_details.name));
				}
            }

        }

    }

    return true;
}
*/
