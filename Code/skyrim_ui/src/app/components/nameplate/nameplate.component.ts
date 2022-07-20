import { Component } from '@angular/core';
import { Observable, startWith } from 'rxjs';
import { ClientService } from '../../services/client.service';


@Component({
  selector: 'app-nameplate',
  templateUrl: './nameplate.component.html',
})
export class NameplateComponent {
  isShown$: Observable<boolean>;
  position$: Observable<number[]>;

  constructor(
    private readonly client: ClientService,

  ) {
    this.position$ = this.client.nameplateChange.pipe(startWith([0, 0]));
  }

}
