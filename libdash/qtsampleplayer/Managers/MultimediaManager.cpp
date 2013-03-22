/*
 * MultimediaManager.cpp
 *****************************************************************************
 * Copyright (C) 2013, bitmovin Softwareentwicklung OG, All Rights Reserved
 *
 * Email: libdash-dev@vicky.bitmovin.net
 *
 * This source code and its use and distribution, is subject to the terms
 * and conditions of the applicable license agreement.
 *****************************************************************************/

#include "MultimediaManager.h"

using namespace libdash::framework::adaptation;
using namespace libdash::framework::buffer;
using namespace sampleplayer::managers;
using namespace sampleplayer::renderer;
using namespace dash::mpd;

#define SEGMENTBUFFER_SIZE 20

MultimediaManager::MultimediaManager            (QTGLRenderer *videoelement, QTAudioRenderer *audioElement) :
                   videoelement                 (videoelement),
                   audioElement                 (audioElement),
                   mpd                          (NULL),
                   period                       (NULL),
                   videoAdaptationSet           (NULL),
                   videoRepresentation          (NULL),
                   videoLogic                   (NULL),
                   videoStream                  (NULL),
                   isStarted                    (false),
                   framesDisplayed              (0),
                   videoSegmentsDownloaded      (0),
                   videoSegmentsDecodingStarted (0),
                   audioAdaptationSet           (NULL),
                   audioRepresentation          (NULL),
                   audioLogic                   (NULL),
                   audioStream                  (NULL)
{
    this->manager = CreateDashManager();
    av_register_all();
}
MultimediaManager::~MultimediaManager           ()
{
    this->Stop();
}

void    MultimediaManager::OnVideoFrameAvailable            (const QImage& image, dash::mpd::IAdaptationSet *adaptationSet)
{
    this->framesDisplayed++;
    this->videoelement->setImage(image);
    this->videoelement->update();
}
void    MultimediaManager::OnAudioSampleAvailable           (const QAudioFormat& format, const char* data, qint64 len)
{
    if (this->audioElement->AudioFormat() != format)
        this->audioElement->SetAudioFormat(format);

    this->audioElement->WriteToBuffer(data, len);
}
IMPD*   MultimediaManager::GetMPD                           ()
{
    return this->mpd;
}
bool    MultimediaManager::Init                             (const std::string& url)
{
    this->mpd = this->manager->Open((char *)url.c_str());
    
    if(this->mpd == NULL)
        return false;

    this->period                = this->mpd->GetPeriods().at(0);
    this->videoAdaptationSet    = this->mpd->GetPeriods().at(0)->GetAdaptationSets().at(0);
    this->videoRepresentation   = this->videoAdaptationSet->GetRepresentation().at(0);

    return true;
}
void    MultimediaManager::Start                            ()
{
    /* Global Start button for start must be added to interface*/
    if(this->isStarted)
    {
        this->Stop();
    }

    this->InitVideoRendering(0);

    this->videoStream->Start();
    //this->audioElement->StartPlayback();
    this->isStarted = true;
}
void    MultimediaManager::Stop                             ()
{
    if(this->isStarted)
    {
        this->videoStream->Stop();
        delete this->videoStream;
        delete this->videoLogic;
        this->videoStream = NULL;
        this->videoLogic = NULL;
    }
    this->isStarted = false;
}
bool    MultimediaManager::SetVideoQuality                  (dash::mpd::IPeriod* period, IAdaptationSet *adaptationSet, dash::mpd::IRepresentation *representation)
{
    if(this->period != period)
    {
        this->period = period;
    }

    this->videoAdaptationSet  = adaptationSet;
    this->videoRepresentation = representation;
    if(this->isStarted)
    {
        int position = this->videoSegmentsDecodingStarted;
        this->Stop();

        this->InitVideoRendering(position);
        this->videoStream->Start();

        this->isStarted = true;
    }
    this->videoRepresentation = representation;

    return true;
}
bool    MultimediaManager::SetAudioQuality                  (dash::mpd::IPeriod* period, IAdaptationSet *adaptationSet, dash::mpd::IRepresentation *representation)
{
    //MUST NOT BE IMPLEMENTED YET
    return false;
}
bool    MultimediaManager::SetVideoAdaptationLogic          (libdash::framework::adaptation::LogicType type)
{
    //Currently unused, always using ManualAdaptation.
    return true;
}
bool    MultimediaManager::SetAudioAdaptationLogic          (libdash::framework::adaptation::LogicType type)
{
    /* MUST NOT BE IMPLEMENTED YET */
    return false;
}
void    MultimediaManager::NotifyVideoObservers             ()
{
    /* Notify DASHPLayer onVideoDataAvailable which does the scaling and forwards the frame to the renderer */
}
void    MultimediaManager::NotifyAudioObservers             ()
{
    /* MUST NOT BE IMPLEMENTED YET */
}
void    MultimediaManager::AttachVideoBufferObserver        (IBufferObserver *videoBufferObserver)
{
    this->videoBufferObservers.push_back(videoBufferObserver);
}
void    MultimediaManager::AttachAudioBufferObserver        (IBufferObserver *audioBufferObserver)
{
}
void    MultimediaManager::NotifyVideoBufferObservers       ()
{
}
void    MultimediaManager::NotifyAudioBufferObservers       ()
{
}
void    MultimediaManager::InitVideoRendering               (uint32_t offset)
{
    this->videoLogic = AdaptationLogicFactory::Create(libdash::framework::adaptation::Manual, this->period, this->videoAdaptationSet, this->mpd);
    this->videoLogic->SetPosition(offset);
    this->videoLogic->SetRepresentation(this->videoRepresentation);
    this->videoLogic->InvokeInitSegment(true);

    this->videoStream = new MultimediaStream(this->videoAdaptationSet, this->videoLogic, SEGMENTBUFFER_SIZE, 0, 0);
    this->videoStream->AttachStreamObserver(this);

    for(int i=0; i < this->videoBufferObservers.size(); i++)
    {
        this->videoStream->AttachBufferObserver(this->videoBufferObservers.at(i));
    }
}
void    MultimediaManager::SetNewQAudioFormat               (const QAudioFormat& format)
{
    /* create new AudioOutput and set format */
}
void    MultimediaManager::OnVideoSegmentDecodingStarted    ()
{
    this->videoSegmentsDecodingStarted++;
}
void    MultimediaManager::OnVideoSegmentDownloaded         ()
{
    this->videoSegmentsDownloaded++;
}
