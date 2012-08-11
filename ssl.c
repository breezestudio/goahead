/*
    ssl.c -- SSL Layer

    Copyright (c) All Rights Reserved. See details at the end of the file.
 */
/********************************* Includes ***********************************/

#include "goahead.h"

#if BIT_PACK_SSL
/******************************************************************************/

int websSSLOpen()
{
    if (sslOpen() < 0) {
        return -1;
    }
    return 0;
}


void websSSLClose()
{
    sslClose();
}


ssize websSSLRead(webs_t wp, char_t *buf, ssize len)
{
    return sslRead(wp, buf, len);
}


/*
    Perform a gets of the SSL socket, returning an allocated string

    Get a string from a socket. This returns data in *buf in a malloced string after trimming the '\n'. If there is
    zero bytes returned, *buf will be set to NULL. If doing non-blocking I/O, it returns -1 for error, EOF or when no
    complete line yet read. If doing blocking I/O, it will block until an entire line is read. If a partial line is
    read socketInputBuffered or socketEof can be used to distinguish between EOF and partial line still buffered. This
    routine eats and ignores carriage returns.
 */
ssize websSSLGets(webs_t wp, char_t **buf)
{ 
    socket_t    *sp;
    ringq_t     *lq;
    char        c;
    ssize       len, nbytes;
    int         sid;
    
    gassert(buf);
    
    *buf = NULL;
    sid = wp->sid;
    
    if ((sp = socketPtr(sid)) == NULL) {
        return -1;
    }
    lq = &sp->lineBuf;
    
    while (1) {
        /* read one byte at a time */
        if ((nbytes = sslRead(wp, &c, 1)) < 0) {
            return -1;
        }
        if (nbytes == 0) {
            /*
                If there is a partial line and we are at EOF, pretend we saw a '\n'
             */
            if (ringqLen(lq) > 0 && (sp->flags & SOCKET_EOF)) {
                c = '\n';
            } else {
                len = ringqLen(lq);
                if (len > 0) {
                    *buf = gallocAscToUni((char *)lq->servp, len);
                } else {
                    *buf = NULL;
                }
                ringqFlush(lq);
                return len;
            }
        }
        /*
            If a newline is seen, return the data excluding the new line to the caller. If carriage return is seen, just
            eat it.  
         */
        if (c == '\n') {
            len = ringqLen(lq);
            if (len > 0) {
                *buf = gallocAscToUni((char *)lq->servp, len);
            } else {
                *buf = NULL;
            }
            ringqFlush(lq);
            return len;
            
        } else if (c == '\r') {
            continue;
        }
        ringqPutcA(lq, c);
    }
    return 0;
}


/*
      Handler for SSL Read Events
 */
static int websSSLReadEvent(webs_t wp)
{
#if MOB
    socket_t    *sp;
    sslConn_t   *sslConn;
    int         ret, sock, resume;
    
    gassert (wp);
    gassert(websValid(wp));
    sp = socketPtr(wp->sid);
    gassert(sp);
    
    sock = sp->sock;
    //  What do do about this
    if (wp->wsp == NULL) {
        resume = 0;
    } else {
        resume = 1;
        sslConn = (wp->wsp)->sslConn;
    }

    /*
        This handler is essentially two back-to-back calls to sslRead.  The first is here in sslAccept where the
        handshake is to take place.  The second is in websReadEvent below where it is expected the client was contacting
        us to send an HTTP request and we need to read that off.
        
        The introduction of non-blocking sockets has made this a little more confusing becuase it is possible to return
        from either of these sslRead calls in a WOULDBLOCK state.  It doesn't ultimately matter, however, because
        sslRead is fully stateful so it doesn't matter how/when it gets invoked later (as long as the correct sslConn is
        used (which is what the resume variable is all about))
    */
    if (sslAccept(&sslConn, sock, sslKeys, resume, NULL) < 0) {
        websTimeoutCancel(wp);
        socketCloseConnection(wp->sid);
        websFree(wp);
        return -1;
    }
    if (resume == 0) {
        (wp->wsp)->sslConn = sslConn;
    }
    websReadEvent(wp);
    return 0;
#else
    return sslAccept(wp);
#endif
}


/*
    The webs socket handler.  Called in response to I/O. We just pass control to the relevant read or write handler. A
    pointer to the webs structure is passed as a (void *) in iwp.
 */
void websSSLSocketEvent(int sid, int mask, void *iwp)
{
    webs_t    wp;
    
    wp = (webs_t) iwp;
    gassert(wp);
    
    if (! websValid(wp)) {
        return;
    }
    if (mask & SOCKET_READABLE) {
        websSSLReadEvent(wp);
    }
    if (mask & SOCKET_WRITABLE) {
        if (wp->writeSocket) {
            (*wp->writeSocket)(wp);
        }
    }
}


ssize websSSLWrite(webs_t wp, char_t *buf, ssize len)
{
    return sslWrite(wp, buf, len);
}


int websSSLEof(webs_t wp)
{
    return socketEof(wp->sid);
}


//  MOB - do we need this. Should it be return int?
void websSSLFlush(webs_t wp)
{
    sslFlush(wp);
}


void websSSLFree(webs_t wp)
{
    sslFree(wp);
}


#endif /* BIT_PACK_SSL */

/*
    @copy   default

    Copyright (c) Embedthis Software LLC, 2003-2012. All Rights Reserved.

    This software is distributed under commercial and open source licenses.
    You may use the Embedthis GoAhead open source license or you may acquire 
    a commercial license from Embedthis Software. You agree to be fully bound
    by the terms of either license. Consult the LICENSE.md distributed with
    this software for full details and other copyrights.

    Local variables:
    tab-width: 4
    c-basic-offset: 4
    End:
    vim: sw=4 ts=4 expandtab

    @end
 */