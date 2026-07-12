# ============================================================
# Preparação do ambiente no Windows - Labirinto (Algoritmos de Busca)
#
# O motor.cpp deste projeto usa X11/Xlib (XReparentWindow) e JAWT no
# modo X11 para embutir a janela do OpenGL dentro da tela Swing. Isso
# é uma API exclusiva do Linux: não existe em compiladores nativos do
# Windows (MSVC/MinGW). Por isso este script NÃO tenta compilar nada
# nativamente no Windows — ele prepara o WSL (Windows Subsystem for
# Linux), que roda um Linux de verdade (com X11 via WSLg) dentro do
# Windows, e a partir daí você usa os MESMOS dois comandos do Linux.
# ============================================================

Write-Host "==> Verificando/instalando o WSL com Ubuntu..." -ForegroundColor Cyan
wsl --install -d Ubuntu

Write-Host ""
Write-Host "==> Se esta e a primeira instalacao do WSL, REINICIE o computador agora." -ForegroundColor Yellow
Write-Host "    Depois de reiniciar, abra o app 'Ubuntu' no menu Iniciar e crie seu usuario/senha Linux."
Write-Host ""
Write-Host "==> Em seguida, DENTRO do terminal Ubuntu (WSL), copie o projeto para lá e rode:" -ForegroundColor Cyan
Write-Host "    chmod +x instalar_dependencias.sh rodar.sh"
Write-Host "    ./instalar_dependencias.sh"
Write-Host "    ./rodar.sh"
Write-Host ""
Write-Host "Observacoes:" -ForegroundColor Cyan
Write-Host " - Windows 10 (build 19044+) e Windows 11 ja trazem o WSLg, que da suporte"
Write-Host "   grafico (X11/Wayland) automatico para o WSL. A janela do simulador vai"
Write-Host "   abrir normalmente na sua area de trabalho do Windows, sem configuracao extra."
Write-Host " - Se estiver numa versao antiga do WSL2 sem WSLg, instale um servidor X"
Write-Host "   (ex.: VcXsrv) no Windows e exporte DISPLAY antes de rodar ./rodar.sh."
Write-Host " - NAO copie o projeto para /mnt/c/... e rode dali: o desempenho de I/O é"
Write-Host "   muito pior. Copie para dentro do filesystem do Linux (ex.: ~/labirinto)."
