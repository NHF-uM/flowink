function uploadImage()
{
    var c=getElm('canvas');
    var w=c.width;
    var h=c.height;
    var p=c.getContext('2d').getImageData(0,0,w,h);
    var n=w*h;
    var a=new Uint8Array((n+1)>>1);   // 每像素4bit，两像素挤一字节
    var i=0, j=0;
    for(var y=0;y<h;y++)for(var x=0;x<w;x++,i++){
        var v=getVal_6color(p,i<<2);
        if(i & 1) a[j++] |= v;        // 奇数x → 低4bit（对齐 epd_set_pixel）
        else      a[j] = v << 4;      // 偶数x → 高4bit
    }

    var xhReq=new XMLHttpRequest();
    xhReq.open('POST','http://'+getElm('ip_addr').value+'/dataUP', true);
    xhReq.onload=xhReq.onerror=function(){ setInn('logTag','Complete!'); };
    xhReq.send(a.buffer);
    setInn('logTag','Uploading...');
}
