set terminal gif animate delay 5
set output 'jacobi.gif'


set size square
unset colorbox
set palette rgb 33,13,10
set yr [] reverse #otherwise it flips the image due to default axis orientation


#WRITE A LOOP HERE TO GO OVER THE FILES AND PLOT THEM ALL. HINT: USE MATRIX WITH IMAGE (GOOGLE DETAILS)
################################
set title "gif jacobi"
#array Data[2]
#Data[1]='00.dat'
#Data[2]='04.dat'
#do for [i=1:2]{plot Data[i] matrix w image}
do for [i=0:50] {
    Datajacobi = sprintf("%d.datjacobi", 0 + 200*i)
    plot Datajacobi matrix with image
}
 
#################################



reset 
set output
set terminal qt #just to return back to normal output terminal; MIGHT NOT BE QT ON YOUR SYSTEM 
 
 
