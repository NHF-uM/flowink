var srcBox,srcImg,dstImg;
var curPal;
// 7.3E: 800x480, 6-color E-Paper (black, white, yellow, red, blue, green) — no orange
var MONO=[[0,0,0],[255,255,255]];
var COLOR6=[[0,0,0],[255,255,255],[255,255,0],[255,0,0],[0,0,255],[0,255,0]];

function getElm(n){return document.getElementById(n);}
function setInn(n,i){ document.getElementById(n).innerHTML=i;}

function processFiles(files){
    var file=files[0];
    var reader=new FileReader();
    srcImg=new Image();
    reader.onload=function(e){
        setInn('srcBox','<img id="imgView" class="sourceImage">');
        var img=getElm('imgView');
        img.src=e.target.result;
        srcImg.src=e.target.result;
    };

    reader.readAsDataURL(file);
}

function drop(e){
    e.stopPropagation();
    e.preventDefault();
    var files=e.dataTransfer.files;
    processFiles(files);
}

function ignoreDrag(e){
    e.stopPropagation();
    e.preventDefault();
}

function getNud(nm,vl){
    return '<td class="comment">'+nm+':</td>'+
    '<td><input id="nud_'+nm+'" class="nud"type="number" value="'+vl+'"/></td>';
}

function Btn(nm,tx,fn){
    return '<div><label class="menu_button" for="_'+nm+'">'+tx+'</label>'+
    '<input class="hidden_input" id="_'+nm+'" type="'+
    (nm==0?'file" onchange="':'button" onclick="')+fn+'"/></div>';
}

window.onload = function(){
    getElm('ip_addr').value = location.hostname;
    srcBox = getElm('srcBox');
    srcBox.ondragenter=ignoreDrag;
    srcBox.ondragover=ignoreDrag;
    srcBox.ondrop=drop;
    srcImg=0;
    curPal=COLOR6;

    setInn('BT',
    Btn(0,'Select image file','processFiles(this.files);')+
    Btn(1,'Level: mono','procImg(true,false);')+
    Btn(2,'Level: color','procImg(true,true);')+
    Btn(3,'Dithering: mono','procImg(false,false);')+
    Btn(4,'Dithering: color','procImg(false,true);')+
    Btn(5,'Upload image','uploadImage();'));

    setInn('XY',getNud('x','0')+getNud('y','0'));
}
